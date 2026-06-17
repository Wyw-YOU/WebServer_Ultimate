#pragma once

#include "net/EventLoop.hpp"
#include "net/EventLoopThread.hpp"

#include <vector>
#include <memory>

class EventLoopThreadPool
{
public:
    EventLoopThreadPool(EventLoop* baseLoop, int threadNum);

    void Start();

    EventLoop* GetNextLoop();

private:
    EventLoop* baseLoop_;   // main loop
    int threadNum_;

    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;

    size_t next_;
};