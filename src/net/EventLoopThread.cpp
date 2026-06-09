#include "net/EventLoopThread.hpp"

EventLoopThread::EventLoopThread()
    : loop_(nullptr)
{
}

EventLoopThread::~EventLoopThread()
{
    if(thread_.joinable())
    {
        thread_.join();
    }
}

EventLoop* EventLoopThread::StartLoop()
{
    thread_ = std::thread(
        &EventLoopThread::ThreadFunc,
        this
    );

    std::unique_lock<std::mutex> lock(mutex_);

    while(loop_ == nullptr)
    {
        cond_.wait(lock);
    }

    return loop_;
}

void EventLoopThread::ThreadFunc()
{
    EventLoop loop;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        loop_ = &loop;  // 资源转移成共享资源
    }
    cond_.notify_one();     // 避免惊群现象

    loop.Loop();
}