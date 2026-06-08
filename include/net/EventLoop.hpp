#pragma once

#include "net/Epoll.hpp"
#include "net/Channel.hpp"
#include "Log.hpp"

#include <mutex>
#include <vector>
#include <functional>
#include <memory>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

class EventLoop
{
public:
    explicit EventLoop(int maxEvents = 1024);
    ~EventLoop();

    void Loop();
    void QueueInLoop(std::function<void()> cb);

    Epoller& GetEpoller();

private:
    void DoPendingFunctors();

    // EventLoop Wakeup
    void HandleWakeup();
    void Wakeup();

private:
    Epoller epoller_;

    std::mutex mutex_;
    std::vector<std::function<void()>> pendingFunctors_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
};