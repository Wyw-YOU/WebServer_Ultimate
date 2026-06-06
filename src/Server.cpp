#include "Server.hpp"

#include <memory>

#define MAXEVENTS 1024

Server::Server(int port)
    : port_(port),
      acceptor_(port),
      epoller_(MAXEVENTS)
    { }

void Server::Start()
{
    // 设置监听套接字为非阻塞，并添加到epoll中
    acceptor_.SetNonBlocking();
    epoller_.AddFd(acceptor_.GetFd(), EPOLLIN);

    LOG_NORMAL("WebServer started on port " + std::to_string(port_));

    while(true)
    {
        // 阻塞等待事件
        int eventCnt = epoller_.Wait(-1);
        if(eventCnt < 0)
        {
            LOG_ERROR("Epoll wait error!");
            continue;
        }

        for(int i = 0; i < eventCnt; ++i)
        {
            // int fd = epoller_.GetEvent(i).data.fd;
            epoll_event ev = epoller_.GetEvent(i);
            int fd = ev.data.fd;
            uint32_t events = ev.events;

            if(fd == acceptor_.GetFd())
            {
                LOG_DEBUG("listen socket event, fd=" + std::to_string(fd));
                // 监听fd可读，说明有新连接到来，处理监听事件
                if(events & (EPOLLIN | EPOLLERR | EPOLLHUP))
                    HandleListenEvent();
            }
            else
            {
                LOG_DEBUG("client socket event, fd=" + std::to_string(fd) + ", events=" + std::to_string(events));
                // 客户端连接
                if(events & (EPOLLERR | EPOLLHUP))
                    CloseConnection(fd);
                else
                {
                    if(events & EPOLLIN)
                        HandleReadEvent(fd);
                    if(events & EPOLLOUT)
                        HandleWriteEvent(fd);
                }
            }
        }
    }
}

//  处理监听事件，接受新连接并添加到epoll中
void Server::HandleListenEvent()
{
    InetAddress clientAddr;

    int connfd = acceptor_.Accept(clientAddr);
    if(connfd < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 没有更多连接了
            return;
        }
        else
        {
            LOG_ERROR("Accept error!");
            return;
        }
    }

    LOG_DEBUG("new connection fd=" + std::to_string(connfd));

    // 设置非阻塞
    Socket clientSock(connfd);
    clientSock.SetNonBlocking();

    // 创建连接对象并加入到map中管理
    connections_[connfd] = std::unique_ptr<Connection>(new Connection(connfd));
    // 添加到epoll中监听读事件（写事件先不做）
    epoller_.AddFd(connfd, EPOLLIN);
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
        epoller_.ModFd(fd, EPOLLOUT | EPOLLIN);
    }
    else    
    {
        // 数据发送完，关闭连接
        CloseConnection(fd);
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
        // 数据发送完，修改为只监听读事件
        epoller_.ModFd(fd, EPOLLIN);
        // 关闭连接(短链接)
        CloseConnection(fd);
    }
    else
    {
        // 发送失败，保持监听写事件，等待下一次可写事件
        LOG_ERROR("Failed to send response to fd=" + std::to_string(fd));
    }
}

//  关闭连接，删除epoll事件并从连接列表中移除
void Server::CloseConnection(int fd)
{
    auto it = connections_.find(fd);

    if(it != connections_.end())
    {
        it->second->Close();
        epoller_.DelFd(fd);
        connections_.erase(it);
        LOG_DEBUG("Closed connection fd=" + std::to_string(fd));
    }
}