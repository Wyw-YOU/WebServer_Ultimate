#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

#include "Log.hpp"

class AsyncLogger
{
public:
    static void Init();
    static void Stop();
    static void Append(LogLevel level, const std::string& msg);

private:
    AsyncLogger();
    ~AsyncLogger();
    void ThreadFunc();

    static AsyncLogger* s_instance;

    std::mutex mutex_;
    std::condition_variable cond_;
    std::vector<std::pair<LogLevel, std::string>> currentBuffer_;
    std::vector<std::pair<LogLevel, std::string>> nextBuffer_;
    std::vector<std::vector<std::pair<LogLevel, std::string>>> flushBuffers_;

    std::thread thread_;
    std::atomic<bool> running_{false};
    static const size_t kBufferThreshold = 1024;
};
