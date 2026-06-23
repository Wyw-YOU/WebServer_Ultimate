#include "Server.hpp"
#include "AsyncLogger.hpp"
#include "db/ConnectionPool.hpp"
#include "util/UrlDecode.hpp"
#include "util/HtmlEscape.hpp"
#include "util/Config.hpp"
#include "util/PasswordUtil.hpp"

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
    std::string safeMessage = HtmlEscape::EscapeJsString(message);
    std::string safeRedirect = HtmlEscape::EscapeHtml(redirect);

    std::ostringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
       << "<title>登录结果</title></head><body>"
       << "<script>alert('" << safeMessage << "');"
       << "location.href='" << safeRedirect << "';</script>"
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
                LOG_ERROR("Database connection pool not initialized");
                resp.SetHtml(BuildAlertPage("服务器内部错误", false));
                return;
            }

            DBGuard guard(pool);
            MYSQL* conn = guard.Get();
            if(!conn)
            {
                LOG_ERROR("Failed to get database connection from pool");
                resp.SetHtml(BuildAlertPage("服务器内部错误", false));
                return;
            }

            // 预处理语句防 SQL 注入
            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            if(!stmt)
            {
                resp.SetHtml(BuildAlertPage("服务器内部错误", false));
                return;
            }

            // 修改 SQL：只查询用户名，密码在 C++ 层验证
            const char* sql = "SELECT id, password FROM users WHERE username=?";
            if(mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0)
            {
                mysql_stmt_close(stmt);
                resp.SetHtml(BuildAlertPage("服务器内部错误", false));
                return;
            }

            MYSQL_BIND bind[1] = {};
            unsigned long usernameLen = username.size();

            bind[0].buffer_type   = MYSQL_TYPE_STRING;
            bind[0].buffer        = const_cast<char*>(username.c_str());
            bind[0].buffer_length = usernameLen;

            mysql_stmt_bind_param(stmt, bind);

            if(mysql_stmt_execute(stmt) != 0)
            {
                mysql_stmt_close(stmt);
                resp.SetHtml(BuildAlertPage("服务器内部错误", false));
                return;
            }

            // 绑定结果：id 和 password
            int userId = 0;
            char storedPassword[100] = {0};
            unsigned long passwordLen = 0;

            MYSQL_BIND result[2] = {};
            result[0].buffer_type = MYSQL_TYPE_LONG;
            result[0].buffer      = &userId;

            result[1].buffer_type = MYSQL_TYPE_STRING;
            result[1].buffer      = storedPassword;
            result[1].buffer_length = sizeof(storedPassword);
            result[1].length = &passwordLen;

            mysql_stmt_bind_result(stmt, result);

            if(mysql_stmt_fetch(stmt) != 0)
            {
                mysql_stmt_close(stmt);
                resp.SetHtml(BuildAlertPage("用户名或密码错误", false));
                return;
            }

            mysql_stmt_close(stmt);

            // 在 C++ 层验证密码（使用 SHA-256 常量时间比较）
            std::string storedHash(storedPassword, passwordLen);
            if(PasswordUtil::VerifyPassword(password, storedHash))
            {
                resp.SetHtml(BuildAlertPage("登录成功，欢迎 " + HtmlEscape::EscapeHtml(username), true));
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

    // 从环境变量读取数据库配置
    const char* dbHost = Config::Get("DB_HOST", "127.0.0.1");
    int dbPort = Config::GetInt("DB_PORT", 3306);
    const char* dbUser = Config::Get("DB_USER", "root");
    const char* dbPassword = Config::Get("DB_PASSWORD");
    const char* dbName = Config::Get("DB_NAME", "webserver");
    int dbPoolSize = Config::GetInt("DB_POOL_SIZE", 8);

    // 检查必需的配置
    if(!dbPassword)
    {
        LOG_ERROR("DB_PASSWORD environment variable is required but not set");
        std::cerr << "Error: DB_PASSWORD environment variable is required" << std::endl;
        std::cerr << "Please set it before starting the server" << std::endl;
        exit(EXIT_FAILURE);
    }

    LOG_NORMAL("Database config: " + std::string(dbUser) + "@" +
               std::string(dbHost) + ":" + std::to_string(dbPort) +
               "/" + std::string(dbName) + " (pool: " + std::to_string(dbPoolSize) + ")");

    // 初始化数据库连接池
    ConnectionPool::Init(dbHost, dbPort, dbUser, dbPassword, dbName, dbPoolSize);

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
