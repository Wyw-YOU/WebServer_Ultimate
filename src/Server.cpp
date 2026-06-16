#include "Server.hpp"

#include <memory>

#define MAXEVENTS 1024
#define THREAD_NUM 20

Server::Server(int port, const std::string& resourceDir)
    : port_(port),
      resourceDir_(resourceDir),
      acceptor_(port),
      loop_(MAXEVENTS),
      pool_(THREAD_NUM)
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
                // 没有更多连接了
                break;
            }
            else
            {
                LOG_ERROR("Accept error!");
                break;
            }
        }

        LOG_DEBUG("new connection fd=" + std::to_string(connfd));

        // 设置非阻塞
        int flags = fcntl(connfd, F_GETFL, 0);
        fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

        // 创建connection 和 channel对象并加入到map中管理
        connections_[connfd] = std::shared_ptr<Connection>(new Connection(connfd, &loop_, resourceDir_, &router_));
        auto conn = connections_[connfd];

        // 绑定回调
        // -----------业务回调
        // 读写回调
        conn->SetOnRead
        (
            [this](std::shared_ptr<Connection> conn)
            {
                HandleReadEvent(conn);
            }
        );

        // 错误回调
        conn->SetOnClose
        (
            [this](std::shared_ptr<Connection> conn)
            {
                CloseConnection(conn);
            }
        );
        // -----------------------------------
        // ------------- IO回调
        std::weak_ptr<Connection> weakConn = conn;  // 避免引起connection环
        conn->SetReadCallback
        (
            [weakConn]()
            {
                if(auto conn = weakConn.lock())
                    conn->HandleRead();
            }
        );

        conn->SetWriteCallback
        (
            [weakConn]()
            {
                if(auto conn = weakConn.lock())
                    conn->HandleWrite();
            }
        );

        conn->SetCloseCallback
        (
            [weakConn]()
            {
                if(auto conn = weakConn.lock())
                    conn->HandleClose();
            }
        );

        // 添加到epoll中监听事件
        conn->EnableReading();

        // 注册到EventLoop中
        loop_.GetEpoller().AddChannel(conn->GetChannel());

        // 添加timer计时器
        loop_.AddTimer(connfd, 60000, 
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
    pool_.AddTask
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

