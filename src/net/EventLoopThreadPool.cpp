#include "net/EventLoopThreadPool.hpp"


EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int threadNum)
    : baseLoop_(baseLoop),
      threadNum_(threadNum),
      next_(0)
    { }

void EventLoopThreadPool::Start()
{
    for(int i = 0; i < threadNum_; ++i)
    {
        std::unique_ptr<EventLoopThread> t(new EventLoopThread());
        EventLoop* loop = t->StartLoop();

        loops_.push_back(loop);
        threads_.push_back(std::move(t));
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop()
{
    if(loops_.empty())
        return baseLoop_;

    EventLoop* loop = loops_[next_];
    next_ = (next_ + 1) % loops_.size();

    return loop;
}