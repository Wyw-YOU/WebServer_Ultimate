#pragma once

#include "net/EventLoop.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoopThread
{
public:
    EventLoopThread();
    ~EventLoopThread();

    EventLoop* StartLoop();

private:
    void ThreadFunc();

private:
    EventLoop* loop_;
    std::thread thread_;

    // 信号量 + 互斥锁
    std::mutex mutex_;
    std::condition_variable cond_;
};