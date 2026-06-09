#pragma once

#include "net/Epoll.hpp"
#include "net/Channel.hpp"
#include "thread/ThreadPool.hpp"
#include "timer/Timer.hpp"
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
    explicit EventLoop(int maxEvents = 1024, int threadNum = 128);
    ~EventLoop();

    void Loop();
    void QueueInLoop(std::function<void()> cb);

    Epoller& GetEpoller();

    // 把任务交给线程池执行
    void RunInThreadPool(std::function<void()> task);

    // 接入timer进行时间管理
    void AddTimer(int fd, int timeoutMs, std::function<void()> cb);
    void AdjustTimer(int fd, int timeoutMs);
    void RemoveTimer(int fd);

private:
    void DoPendingFunctors();

    // EventLoop Wakeup
    void HandleWakeup();
    void Wakeup();

private:
    Timer timer_;
    Epoller epoller_;

    std::mutex mutex_;
    std::vector<std::function<void()>> pendingFunctors_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ThreadPool threadPool_;
};