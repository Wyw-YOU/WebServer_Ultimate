#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>

#include "Log.hpp"

class AsyncLogger
{
public:
    static void Init(const std::string& logDir = "./logs",
                     size_t maxFileSize = 50 * 1024 * 1024,
                     int maxFiles = 10);
    static void Stop();
    static void Append(LogLevel level, const std::string& msg);

private:
    AsyncLogger();
    ~AsyncLogger();
    void ThreadFunc();
    void OpenLogFile();
    void RotateLog();

    static AsyncLogger* s_instance;

    std::mutex mutex_;
    std::condition_variable cond_;
    std::vector<std::pair<LogLevel, std::string>> currentBuffer_;
    std::vector<std::pair<LogLevel, std::string>> nextBuffer_;
    std::vector<std::vector<std::pair<LogLevel, std::string>>> flushBuffers_;

    std::thread thread_;
    std::atomic<bool> running_{false};
    static const size_t kBufferThreshold = 1024;

    // 文件日志
    std::string logDir_;
    size_t maxFileSize_;
    int maxFiles_;
    std::ofstream logFile_;
    size_t currentFileSize_ = 0;
};
