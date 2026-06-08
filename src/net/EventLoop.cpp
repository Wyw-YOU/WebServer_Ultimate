#include "net/EventLoop.hpp"

EventLoop::EventLoop(int maxEvents)
    : epoller_(maxEvents)
{
    // 创建 eventfd
    wakeupFd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(wakeupFd_ < 0)
    {
        LOG_ERROR("Faild to create eventfd: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    // 封装 wakeupfd 为 Channel
    wakeupChannel_ = std::unique_ptr<Channel>(new Channel(wakeupFd_));
    wakeupChannel_->SetReadCallback
    (
        [this]()
        {
            HandleWakeup();
        }
    );
    wakeupChannel_->SetEvents(EPOLLIN | EPOLLET);
}
EventLoop::~EventLoop()
{
    if(wakeupFd_ >= 0)
    {
        close(wakeupFd_);
    }
}

Epoller& EventLoop::GetEpoller()
{
    return epoller_;
}

void EventLoop::Loop()
{
    while(true)
    {
        int eventCnt = epoller_.Wait(-1);
        if(eventCnt < 0)
        {
            LOG_ERROR("epoll_wait failed: " + std::string(strerror(errno)));
            continue;
        }

        for(int i = 0; i < eventCnt; ++i)
        {
            epoll_event ev = epoller_.GetEvent(i);
            Channel* channel = static_cast<Channel*>(ev.data.ptr);
            if(channel == nullptr)
                continue;

            channel->SetRevents(ev.events);
            channel->HandleEvent();
        }

        DoPendingFunctors();
    }
}

// 将函数假如到任务队列，并唤醒Loop
void EventLoop::QueueInLoop(std::function<void()> cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    // 唤醒eventloop线程
    Wakeup();
}

void EventLoop::DoPendingFunctors()
{
    std::vector<std::function<void()>> funcs;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        funcs.swap(pendingFunctors_);
    }

    for(auto& func : funcs)
    {
        func();
    }
}

// 向 eventfd 写数据，唤醒 epoll_wait
void EventLoop::Wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::Wakeup failed: " + std::string(strerror(errno)));
    }
}

// 读取 eventfd 数据，清空计数
void EventLoop::HandleWakeup()
{
    uint64_t one;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::HandleWakeup read error: " + std::string(strerror(errno)));
    }
}