#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threadCount = 128);

    ~ThreadPool();

    void AddTask(std::function<void()> task);

private:
    void Worker();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex mutex_;
    std::condition_variable cond_;

    bool stop_;
};