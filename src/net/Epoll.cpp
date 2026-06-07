#include "net/Epoll.hpp"

Epoller::Epoller(int maxEvents)
    : epollFd_(epoll_create1(0)),
      events_(maxEvents)
{
    if(epollFd_ < 0)
    {
        Error::SysError("Failed to create epoll instance!");
    }
}

Epoller::~Epoller()
{
    if(epollFd_ >= 0)
    {
        close(epollFd_);
    }
}

bool Epoller::AddFd(int fd, uint32_t events)
{
    epoll_event ev;

    ev.data.fd = fd;
    ev.events = events;

    return epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == 0;
}
bool Epoller::ModFd(int fd, uint32_t events)
{
    epoll_event ev;

    ev.data.fd = fd;
    ev.events = events;

    return epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) == 0;
}
bool Epoller::DelFd(int fd)
{
    return epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == 0;
}

bool Epoller::AddChannel(Channel* channel)
{
    epoll_event ev;
    ev.data.ptr = channel;
    ev.events = channel->GetEvents();

    return epoll_ctl(epollFd_, EPOLL_CTL_ADD, channel->GetFd(), &ev) == 0;
}
bool Epoller::ModChannel(Channel* channel)
{
    epoll_event ev;
    ev.data.ptr = channel;
    ev.events = channel->GetEvents();

    return epoll_ctl(epollFd_, EPOLL_CTL_MOD, channel->GetFd(), &ev) == 0;
}
bool Epoller::DelChannel(Channel* channel)
{
    return epoll_ctl(epollFd_, EPOLL_CTL_DEL, channel->GetFd(), nullptr) == 0;
}

int Epoller::Wait(int timeoutMs)
{
    return epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
}

epoll_event Epoller::GetEvent(size_t index) const
{
    if(index < events_.size())
    {
        return events_[index];
    }
    else
    {
        Error::SysError("Index out of bounds in GetEvent!");
        return epoll_event{};
    }
}