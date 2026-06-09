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
        connections_[connfd] = std::shared_ptr<Connection>(new Connection(connfd, resourceDir_));
        auto conn = connections_[connfd];

        // 绑定回调
        // 读回调
        conn->SetReadCallback
        (
            [this, connfd]()
            {
                HandleReadEvent(connfd);
            }
        );
        // 写回调
        conn->SetWriteCallback
        (
            [this, connfd]()
            {
                HandleWriteEvent(connfd);
            }
        );
        // 错误回调
        conn->SetCloseCallback
        (
            [this, connfd]()
            {
                CloseConnection(connfd);
            }
        );

        // 添加到epoll中监听事件
        connections_[connfd].channel_.get()->SetEvents(EPOLLIN | EPOLLET);

        // 注册到EventLoop中
        loop_.GetEpoller().AddChannel(channel);

        // 添加timer计时器
        loop_.AddTimer(connfd, 5000, 
            [this, connfd]()
            {
                CloseConnection(connfd);
            }
        );
    }
}

void Server::HandleFinished(int fd, std::shared_ptr<Connection> conn)
{
    //  是否长连接？
    // 长连接：继续监听
    if(conn->IsKeepAlive()) 
    {
        // auto channel = channels_[fd].get();
        auto channel = connections_[fd]->GetChannel();
        channel->SetEvents(EPOLLIN | EPOLLET);
        loop_.GetEpoller().ModChannel(channel);

        conn->SetState(ConnState::Connected);
    } 
    else 
    {
        conn->SetState(ConnState::Closed);
        // 短链接：关闭
        CloseConnection(fd);
    }
}

//  处理读事件，读取客户端请求数据并写入Buffer，构建Http请求并返回HTTP响应，发送HTTP响应数据，并关闭连接
void Server::HandleReadEvent(int fd)
{
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        LOG_DEBUG("Connection not found for fd = " + std::to_string(fd));
        return;
    }
    // 刷新计时
    loop_.AdjustTimer(fd, 60000);

    std::shared_ptr<Connection> conn = it->second;
    if(conn->GetState() != ConnState::Connected)
    {
        LOG_DEBUG("Connection is processing, ignore fd= " + std::to_string(fd));
        return;
    }

    // 读取失败则关闭
    if(!conn->Read())
    {
        CloseConnection(fd);
        return;
    }
    LOG_DEBUG("Read data from fd=" + std::to_string(fd));
    conn->SetState(ConnState::Processing);

    // 业务处理(丢给线程池)
    pool_.AddTask([this, conn, fd]() 
    {
        // 线程池线程执行
        conn->Process();
        conn->SetState(ConnState::Writing);
    
        // Process 完之后，把写事件提交回主线程 EventLoop
        loop_.QueueInLoop([this, conn, fd]() 
        {
            // 尝试发送响应
            WriteResult result = conn->Write();

            switch(result)
            {
                case WRITE_COMPLETE:
                {
                    // 判断是否是长连接
                    HandleFinished(fd, conn);
                    break;
                }

                case WRITE_AGAIN:
                {
                    // 数据未发送完，添加 EPOLLOUT
                    // auto channel = channels_[fd].get();
                    auto channel = connections_[fd]->GetChannel();
                    channel->SetEvents(EPOLLIN | EPOLLOUT | EPOLLET);
                    loop_.GetEpoller().ModChannel(channel);

                    break;
                }

                case WRITE_ERROR:
                {
                    CloseConnection(fd);
                    break;
                }
            }
        });
    });
}

// 处理写事件，继续发送响应数据
void Server::HandleWriteEvent(int fd)
{
    // 刷新计时
    loop_.AdjustTimer(fd, 60000);

    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        LOG_DEBUG("Connection not found for fd = " + std::to_string(fd));
        return;
    }

    std::shared_ptr<Connection> conn = it->second;
    LOG_DEBUG("Handle write event for fd=" + std::to_string(fd));

    auto result = conn->Write();
    switch(result)
    {
        case WRITE_COMPLETE:
        {
            // 数据发送完
            // 判断是否是长连接
            HandleFinished(fd, conn);
            break;
        }
        case WRITE_AGAIN:
        {
            // 未发送完，继续监听写事件
            // auto channel = channels_[fd].get();
            auto channel = connections_[fd]->GetChannel();
            channel->SetEvents(EPOLLIN | EPOLLOUT | EPOLLET);

            loop_.GetEpoller().ModChannel(channel);
            LOG_DEBUG("Waiting to send more data to fd=" + std::to_string(fd));
            break;
        }
        case WRITE_ERROR:
        {
            CloseConnection(fd);
            break;
        }
    }
}

//  关闭连接，删除epoll事件并从连接列表中移除
void Server::CloseConnection(int fd)
{
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