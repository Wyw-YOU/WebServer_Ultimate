#include "net/Channel.hpp"

Channel::Channel(int fd, bool et)
    : fd_(fd),
      events_(0),
      revents_(0),
      et_(et ? EPOLLET : 0)
{}

void Channel::HandleEvent()
{
    if(revents_ & (EPOLLERR | EPOLLHUP))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
        return;
    }

    if(revents_ & EPOLLIN)
    {
        if(readCallback_)
        {
            readCallback_();
        }
    }

    if(revents_ & EPOLLOUT)
    {
        LOG_DEBUG("EPOLLOUT triggered fd=" + std::to_string(fd_));
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}

void Channel::SetReadCallback(EventCallback cb)
{
    readCallback_ = std::move(cb);
}
void Channel::SetWriteCallback(EventCallback cb)
{
    writeCallback_ = std::move(cb);
}
void Channel::SetCloseCallback(EventCallback cb)
{
    closeCallback_ = std::move(cb);
}

int Channel::GetFd() const
{
    return fd_;
}
uint32_t Channel::GetEvents() const
{
    return events_;
}
uint32_t Channel::GetRevents() const
{
    return revents_;
}

uint32_t Channel::GetEtFlag() const
{
    return et_;
}

void Channel::SetEvents(uint32_t events)
{
    events_ = events;
}
void Channel::SetRevents(uint32_t revents)
{
    revents_ = revents;
}

void Channel::EnableReading()
{
    events_ |= EPOLLIN;
}

void Channel::EnableWriting()
{
    events_ |= EPOLLOUT;
}

void Channel::DisableWriting()
{
    events_ &= ~EPOLLOUT;
}

void Channel::DisableAll()
{
    events_ = 0;
}