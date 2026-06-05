#include "net/Socket.hpp"
#include "Log.hpp"

// 构造
Socket::Socket()
{
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
}

Socket::Socket(int fd)
    : fd_(fd)
    { }

// 析构
Socket::~Socket()
{
    Close();
}

// 绑定
void Socket::Bind(const sockaddr_in* addr)
{
    if(bind(fd_, reinterpret_cast<const sockaddr*>(addr), sizeof(sockaddr_in)) < 0)
    {
        LOG_ERROR("Bind error!");
    }
}

// 监听
void Socket::Listen(int backlog)
{
    if(listen(fd_, backlog) < 0)
    {
        LOG_ERROR("Listen error!");
    }
}

// 连接
int Socket::Accept(sockaddr_in* addr)
{
    socklen_t len = sizeof(sockaddr_in);

    return accept(fd_, reinterpret_cast<sockaddr*>(addr), &len);
}

// 获取fd
int Socket::GetFd() const
{
    return fd_;
}

void Socket::Close()
{
    if(fd_ != -1)
    {
        close(fd_);
        fd_ = -1;
    }
}