#include "Server.hpp"
#include "AsyncLogger.hpp"
#include "db/ConnectionPool.hpp"
#include "util/UrlDecode.hpp"

#include <mysql/mysql.h>
#include <memory>
#include <sstream>

#define MAXEVENTS 1024
#define THREAD_NUM 20

Server* Server::instance_ = nullptr;

// 构建登录结果提示页面
static std::string BuildAlertPage(const std::string& message, bool success)
{
    std::string redirect = success ? "/hello.html" : "/";
    std::ostringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
       << "<title>登录结果</title></head><body>"
       << "<script>alert('" << message << "');"
       << "location.href='" << redirect << "';</script>"
       << "</body></html>";
    return ss.str();
}

Server::Server(int port, const std::string& resourceDir)
    : port_(port),
      resourceDir_(resourceDir),
      acceptor_(port),
      loop_(MAXEVENTS),
      ioPool_(&loop_, THREAD_NUM)
{
    router_.Get("/hello",
        [](const HttpRequest& req, HttpResponse& resp)
        {
            resp.SetHtml("<h1>Hello Router</h1>");
        });

    router_.Post("/login",
        [](const HttpRequest& req, HttpResponse& resp)
        {
            auto params = ParseFormBody(req.Body());
            std::string username = params["username"];
            std::string password = params["password"];

            if(username.empty() || password.empty())
            {
                resp.SetHtml(BuildAlertPage("用户名和密码不能为空", false));
                return;
            }

            auto pool = ConnectionPool::Instance();
            if(!pool)
            {
                resp.SetHtml(BuildAlertPage("数据库未初始化", false));
                return;
            }

            DBGuard guard(pool);
            MYSQL* conn = guard.Get();
            if(!conn)
            {
                resp.SetHtml(BuildAlertPage("数据库连接失败", false));
                return;
            }

            // 预处理语句防 SQL 注入
            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            if(!stmt)
            {
                resp.SetHtml(BuildAlertPage("服务器内部错误", false));
                return;
            }

            const char* sql = "SELECT id FROM users WHERE username=? AND password=?";
            if(mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0)
            {
                mysql_stmt_close(stmt);
                resp.SetHtml(BuildAlertPage("服务器内部错误", false));
                return;
            }

            MYSQL_BIND bind[2] = {};
            unsigned long usernameLen = username.size();
            unsigned long passwordLen = password.size();

            bind[0].buffer_type   = MYSQL_TYPE_STRING;
            bind[0].buffer        = const_cast<char*>(username.c_str());
            bind[0].buffer_length = usernameLen;

            bind[1].buffer_type   = MYSQL_TYPE_STRING;
            bind[1].buffer        = const_cast<char*>(password.c_str());
            bind[1].buffer_length = passwordLen;

            mysql_stmt_bind_param(stmt, bind);

            if(mysql_stmt_execute(stmt) != 0)
            {
                mysql_stmt_close(stmt);
                resp.SetHtml(BuildAlertPage("查询失败", false));
                return;
            }

            int userId = 0;
            MYSQL_BIND result[1] = {};
            result[0].buffer_type = MYSQL_TYPE_LONG;
            result[0].buffer      = &userId;

            mysql_stmt_bind_result(stmt, result);
            bool found = (mysql_stmt_fetch(stmt) == 0);

            mysql_stmt_close(stmt);

            if(found)
            {
                resp.SetHtml(BuildAlertPage("登录成功，欢迎 " + username, true));
            }
            else
            {
                resp.SetHtml(BuildAlertPage("用户名或密码错误", false));
            }
        });

    acceptor_.SetNonBlocking();

    listenChannel_.reset(new Channel(acceptor_.GetFd()));
    listenChannel_->SetEvents(EPOLLIN | EPOLLET);

    listenChannel_->SetReadCallback(
        [this]()
        {
            HandleListenEvent();
        }
    );

    loop_.GetEpoller().AddChannel(listenChannel_.get());
    instance_ = this;
}

void Server::SignalHandler(int sig)
{
    (void)sig;
    if(instance_)
    {
        instance_->running_.store(false);
        instance_->loop_.Quit();
    }
}

void Server::Start()
{
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    AsyncLogger::Init();

    // 初始化数据库连接池
    ConnectionPool::Init("127.0.0.1", 3306, "root", "Wyw962464.", "webserver", 8);

    LOG_NORMAL("WebServer started on port " + std::to_string(port_));
    ioPool_.Start();
    loop_.Loop();

    // 优雅退出：先销毁连接池，再停止日志
    ConnectionPool::Destroy();
    AsyncLogger::Stop();
}

void Server::HandleListenEvent()
{
    InetAddress clientAddr;

    while(true)
    {
        int connfd = acceptor_.Accept(clientAddr);
        if(connfd < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                LOG_DEBUG("No more incoming connections to accept.");
                break;
            }
            LOG_ERROR("Accept error!");
            break;
        }

        LOG_DEBUG("new connection fd=" + std::to_string(connfd));

        int flags = fcntl(connfd, F_GETFL, 0);
        fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

        EventLoop* ioLoop = ioPool_.GetNextLoop();

        // 优先从连接池复用，否则新建
        Connection* raw = ioLoop->GetConnectionFromPool();
        if(raw)
            raw->Reuse(connfd, ioLoop, resourceDir_, &router_);
        else
            raw = new Connection(connfd, ioLoop, resourceDir_, &router_);

        // 自定义 deleter：关闭 fd 并归还连接到池
        std::shared_ptr<Connection> conn(raw,
            [ioLoop](Connection* c)
            {
                c->Close();
                ioLoop->ReturnConnectionToPool(c);
            });

        ioLoop->AddConnection(conn);

        std::weak_ptr<Connection> weakConn = conn;

        conn->SetReadCallback([weakConn]()
        {
            if(auto c = weakConn.lock())
                c->HandleRead();
        });

        conn->SetWriteCallback([weakConn]()
        {
            if(auto c = weakConn.lock())
                c->HandleWrite();
        });

        conn->SetCloseCallback([weakConn]()
        {
            if(auto c = weakConn.lock())
                c->HandleClose();
        });

        conn->EnableReading();
        ioLoop->GetEpoller().AddChannel(conn->GetChannel());

        ioLoop->AddTimer(connfd, 60000,
            [ioLoop, connfd]()
            {
                ioLoop->QueueInLoop([ioLoop, connfd]()
                {
                    ioLoop->RemoveConnection(connfd);
                });
            }
        );
    }
}
