#include "Server.hpp"

#include <memory>

#define MAXEVENTS 1024
#define THREAD_NUM 20

Server::Server(int port, const std::string& resourceDir)
    : port_(port),
      resourceDir_(resourceDir),
      acceptor_(port),
      loop_(MAXEVENTS),
      workerPool_(THREAD_NUM),          // worker pool
      ioPool_(&loop_, THREAD_NUM) // io pool
{
    router_.Get("/hello",
        [](const HttpRequest& req, HttpResponse& resp)
        {
            resp.SetHtml("<h1>Hello Router</h1>");
        });

    router_.Post("/login",
        [](const HttpRequest& req, HttpResponse& resp)
        {
            resp.SetText("POST BODY:\n" + req.Body());
        });

    // 先设置非阻塞，再注册到epoll
    acceptor_.SetNonBlocking();

    listenChannel_.reset(new Channel(acceptor_.GetFd()));
    listenChannel_->SetEvents(EPOLLIN | EPOLLET);

    listenChannel_->SetReadCallback
    (
        [this]()
        {
            HandleListenEvent();
        }
    );

    loop_.GetEpoller().AddChannel(listenChannel_.get());
}

// 启动服务器
void Server::Start()
{
    LOG_NORMAL("WebServer started on port " + std::to_string(port_));
    ioPool_.Start();
    // 进入实际循坏
    loop_.Loop();
}


//  处理监听事件，接受新连接并添加到epoll中
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

        // 非阻塞
        int flags = fcntl(connfd, F_GETFL, 0);
        fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

        // ⭐ 关键：选择 IO 线程
        EventLoop* ioLoop = ioPool_.GetNextLoop();

        // ⭐ 关键：Connection 绑定 IO loop（不是 main loop）
        auto conn = std::make_shared<Connection>(
            connfd,
            ioLoop,
            resourceDir_,
            &router_
        );

        connections_[connfd] = conn;

        // ========== 业务回调 ==========
        conn->SetOnRead([this](std::shared_ptr<Connection> conn)
        {
            HandleReadEvent(conn);
        });

        conn->SetOnClose([this](std::shared_ptr<Connection> conn)
        {
            CloseConnection(conn);
        });

        // ========== IO 回调 ==========
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

        // 注册 epoll
        conn->EnableReading();
        ioLoop->GetEpoller().AddChannel(conn->GetChannel());

        // ⭐ Timer 必须挂在 ioLoop（关键修复）
        ioLoop->AddTimer(connfd, 60000,
            [this, connfd]()
            {
                auto it = connections_.find(connfd);
                if(it != connections_.end())
                    CloseConnection(it->second);
            }
        );
    }
}


//  处理读事件，读取客户端请求数据并写入Buffer，构建Http请求并返回HTTP响应，发送HTTP响应数据，并关闭连接
void Server::HandleReadEvent(std::shared_ptr<Connection> conn)
{
    int fd = conn->GetFd();
    // 刷新计时
    loop_.AdjustTimer(fd, 60000);

    // std::shared_ptr<Connection> conn = it->second;
    if(conn->GetState() != ConnState::Connected)
    {
        LOG_DEBUG("Connection is processing, ignore fd= " + std::to_string(fd));
        return;
    }

    LOG_DEBUG("Read data from fd=" + std::to_string(fd));
    conn->SetState(ConnState::Processing);

    // 业务处理(丢给线程池，并下沉到Connection层，让Connection自行管理)
    workerPool_.AddTask
    (
        [conn]()
        {
            conn->ProcessInWorker();
        }
    );
}


//  关闭连接，删除epoll事件并从连接列表中移除
void Server::CloseConnection(std::shared_ptr<Connection> conn)
{
    // 避免： HandleClose()、Timer超时、Write失败 同时进入关闭流程
    if(!conn)
        return;

    if(conn->GetState() == ConnState::Closed)
        return;

    conn->SetState(ConnState::Closed);
    int fd = conn->GetFd();
    auto it = connections_.find(fd);
    if(it != connections_.end())
    {
        // 先移除epoll里面的fd
        auto channel = it->second->GetChannel();
        loop_.GetEpoller().DelChannel(channel);

        it->second->Close();
        connections_.erase(it);
    }
    // 关闭计时
    loop_.RemoveTimer(fd);

    LOG_DEBUG("Closed connection fd=" + std::to_string(fd));
}

