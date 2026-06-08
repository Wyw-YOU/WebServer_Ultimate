#include "Server.hpp"

#include <memory>

#define MAXEVENTS 1024

Server::Server(int port, const std::string& resourceDir)
    : port_(port),
      resourceDir_(resourceDir),
      acceptor_(port),
      loop_(MAXEVENTS)
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
        connections_[connfd] = std::unique_ptr<Connection>(new Connection(connfd, resourceDir_));
        channels_[connfd] = std::unique_ptr<Channel>(new Channel(connfd));

        // 绑定回调
        Channel* channel = channels_[connfd].get();
        // 读回调
        channel->SetReadCallback
        (
            [this, connfd]()
            {
                HandleReadEvent(connfd);
            }
        );
        // 写回调
        channel->SetWriteCallback
        (
            [this, connfd]()
            {
                HandleWriteEvent(connfd);
            }
        );
        // 错误回调
        channel->SetCloseCallback
        (
            [this, connfd]()
            {
                CloseConnection(connfd);
            }
        );

        // 添加到epoll中监听事件
        channel->SetEvents(EPOLLIN | EPOLLET);

        // 注册到EventLoop中
        loop_.GetEpoller().AddChannel(channel);
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

    Connection* conn = it->second.get();
    // 读取失败则关闭
    if(!conn->Read())
    {
        CloseConnection(fd);
        return;
    }
    LOG_DEBUG("Read data from fd=" + std::to_string(fd));
    // 业务处理
    conn->Process();

    // 发送响应
    if(!conn->Write())
    {
        // 数据未发送完，添加EPOLLOUT事件，等待下一次可写事件
        auto channel = channels_[fd].get();
        channel->SetEvents(EPOLLIN | EPOLLOUT | EPOLLET);

        loop_.GetEpoller().ModChannel(channel);
    }
    else    
    {
        // 数据发送完，关闭连接
        if(conn->IsKeepAlive())
        {
            // 长连接，继续监听读事件
            auto channel = channels_[fd].get();
            channel->SetEvents(EPOLLIN | EPOLLET);
    
            loop_.GetEpoller().ModChannel(channel);
        }
        else
        {
            // 短连接，关闭连接
            CloseConnection(fd);
        }
    }
}

// 处理写事件，继续发送响应数据
void Server::HandleWriteEvent(int fd)
{
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        LOG_DEBUG("Connection not found for fd = " + std::to_string(fd));
        return;
    }

    Connection* conn = it->second.get();
    LOG_DEBUG("Handle write event for fd=" + std::to_string(fd));
    if(conn->Write())
    {
        // 数据发送完
        if(conn->IsKeepAlive())
        {
            // 长连接，继续监听读事件
            auto channel = channels_[fd].get();
            channel->SetEvents(EPOLLIN | EPOLLET);
    
            loop_.GetEpoller().ModChannel(channel);
        }
        else
        {
            // 短连接，关闭连接
            CloseConnection(fd);
        }
    }
    else
    {
        // 未发送完，继续监听写事件
        auto channel = channels_[fd].get();
        channel->SetEvents(EPOLLIN | EPOLLOUT | EPOLLET);

        loop_.GetEpoller().ModChannel(channel);
        LOG_DEBUG("Waiting to send more data to fd=" + std::to_string(fd));
    }
}

//  关闭连接，删除epoll事件并从连接列表中移除
void Server::CloseConnection(int fd)
{
    // 先从epoll注销Channel，再关闭fd，避免对已关闭fd执行epoll DEL
    auto cit = channels_.find(fd);
    if(cit != channels_.end())
    {
        loop_.GetEpoller().DelChannel(cit->second.get());
        channels_.erase(cit);
    }

    auto it = connections_.find(fd);
    if(it != connections_.end())
    {
        it->second->Close();
        connections_.erase(it);
    }

    LOG_DEBUG("Closed connection fd=" + std::to_string(fd));
}