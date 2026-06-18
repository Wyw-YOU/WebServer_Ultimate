#include "Server.hpp"

#include <memory>

#define MAXEVENTS 1024
#define THREAD_NUM 20

Server::Server(int port, const std::string& resourceDir)
    : port_(port),
      resourceDir_(resourceDir),
      acceptor_(port),
      loop_(MAXEVENTS),
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
        auto conn = std::make_shared<Connection>(connfd, ioLoop, resourceDir_, &router_);
        ioLoop->AddConnection(conn);

        // ========== IO 回调（业务处理已在 HandleRead 中直接执行） ==========
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

        // Timer 必须挂在 ioLoop
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
