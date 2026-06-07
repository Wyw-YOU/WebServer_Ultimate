#include "net/Socket.hpp"
#include "util/Error.hpp"
#include "Log.hpp"

#include <cstdlib>

// 构造
Socket::Socket()
{
    fd_ = socket(AF_INET, SOCK_STREAM, 0);

    LOG_DEBUG("Create socket fd=" + std::to_string(fd_));
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
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(fd_, reinterpret_cast<const sockaddr*>(addr), sizeof(sockaddr_in)) < 0)
    {
        Error::SysError("Bind error");
        exit(1);
    }
}

// 监听
void Socket::Listen(int backlog)
{
    if(listen(fd_, backlog) < 0)
    {
        Error::SysError("Listen error");
        exit(1);
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

// 设置非阻塞
void Socket::SetNonBlocking()
{
    int flags =fcntl(fd_, F_GETFL, 0);
    if(flags < 0)
    {
        Error::SysError("fcntl get");
        return;
    }

    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
}