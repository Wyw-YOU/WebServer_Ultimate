#include "thread/ThreadPool.hpp"


ThreadPool::ThreadPool(size_t threadCount)
    : stop_(false)
{
    for (size_t i = 0; i < threadCount; ++i) 
    {
        workers_.emplace_back(&ThreadPool::Worker, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cond_.notify_all();   // 唤醒所有工作线程

    for (auto& worker : workers_) 
    {
        if(worker.joinable()) 
        {
            worker.join();
        }
    }
}

void ThreadPool::AddTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    cond_.notify_one();   // 通知一个空闲线程
}

void ThreadPool::Worker()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 等待条件：有任务 或 线程池停止
            cond_.wait
            (
                lock, [this] 
                {
                    return !tasks_.empty() || stop_;
                }
            );

            if (stop_ && tasks_.empty()) 
            {
                return;   // 线程退出
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        // 在锁外执行任务，避免阻塞其他线程
        if (task) 
        {
            task();
        }
    }
}