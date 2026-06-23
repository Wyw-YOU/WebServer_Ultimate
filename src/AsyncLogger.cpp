#include "AsyncLogger.hpp"
#include "Log.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

AsyncLogger* AsyncLogger::s_instance = nullptr;

AsyncLogger::AsyncLogger() = default;
AsyncLogger::~AsyncLogger() = default;

void AsyncLogger::Init(const std::string& logDir, size_t maxFileSize, int maxFiles)
{
    if(s_instance)
        return;

    s_instance = new AsyncLogger();
    s_instance->logDir_ = logDir;
    s_instance->maxFileSize_ = maxFileSize;
    s_instance->maxFiles_ = maxFiles;

    s_instance->currentBuffer_.reserve(kBufferThreshold);
    s_instance->nextBuffer_.reserve(kBufferThreshold);

    s_instance->OpenLogFile();

    s_instance->running_.store(true);
    s_instance->thread_ = std::thread(&AsyncLogger::ThreadFunc, s_instance);
}

void AsyncLogger::OpenLogFile()
{
    // 创建日志目录
    mkdir(logDir_.c_str(), 0755);

    std::string path = logDir_ + "/server.log";
    logFile_.open(path, std::ios::app);
    if(logFile_.is_open())
    {
        // 获取当前文件大小
        logFile_.seekp(0, std::ios::end);
        currentFileSize_ = logFile_.tellp();
    }
    else
    {
        std::cerr << "[AsyncLogger] Failed to open log file: " << path << std::endl;
    }
}

void AsyncLogger::RotateLog()
{
    logFile_.close();

    // 删除最旧的文件
    std::string oldest = logDir_ + "/server.log." + std::to_string(maxFiles_ - 1);
    unlink(oldest.c_str());

    // server.log.(N-2) → server.log.(N-1), ..., server.log.1 → server.log.2
    for(int i = maxFiles_ - 2; i >= 1; --i)
    {
        std::string from = logDir_ + "/server.log." + std::to_string(i);
        std::string to   = logDir_ + "/server.log." + std::to_string(i + 1);
        rename(from.c_str(), to.c_str());
    }

    // server.log → server.log.1
    std::string current = logDir_ + "/server.log";
    std::string backup  = logDir_ + "/server.log.1";
    rename(current.c_str(), backup.c_str());

    // 重新打开新文件
    OpenLogFile();
}

void AsyncLogger::Stop()
{
    if(!s_instance)
        return;

    {
        std::lock_guard<std::mutex> lock(s_instance->mutex_);
        s_instance->running_.store(false);
    }
    s_instance->cond_.notify_one();
    s_instance->thread_.join();

    // flush 剩余 buffer
    auto flush = [&](std::vector<std::pair<LogLevel, std::string>>& buf)
    {
        for(auto& entry : buf)
        {
            std::ostream& os = (entry.first == LOG_ERROR) ? std::cerr : std::cout;
            os << entry.second << '\n';
            if(s_instance->logFile_.is_open())
                s_instance->logFile_ << entry.second << '\n';
        }
    };

    for(auto& buf : s_instance->flushBuffers_)
        flush(buf);
    flush(s_instance->currentBuffer_);

    std::cout.flush();
    std::cerr.flush();
    if(s_instance->logFile_.is_open())
        s_instance->logFile_.flush();

    delete s_instance;
    s_instance = nullptr;
}

void AsyncLogger::Append(LogLevel level, const std::string& msg)
{
    if(!s_instance)
    {
        std::ostream& os = (level == LOG_ERROR) ? std::cerr : std::cout;
        os << "[" << Log::LevelName(level) << "] " << msg << std::endl;
        return;
    }

    // 格式化时间戳
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    char line[512];
    int len = snprintf(line, sizeof(line), "%s.%03d [%s] %s",
                        timeBuf, (int)ms.count(), Log::LevelName(level), msg.c_str());

    std::lock_guard<std::mutex> lock(s_instance->mutex_);
    s_instance->currentBuffer_.emplace_back(level, std::string(line, len));

    if(s_instance->currentBuffer_.size() >= kBufferThreshold)
    {
        s_instance->flushBuffers_.push_back(std::move(s_instance->currentBuffer_));
        if(!s_instance->nextBuffer_.empty())
        {
            s_instance->currentBuffer_ = std::move(s_instance->nextBuffer_);
        }
        s_instance->cond_.notify_one();
    }
}

void AsyncLogger::ThreadFunc()
{
    while(running_.load())
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait_for(lock, std::chrono::seconds(3),
                [this]{ return !flushBuffers_.empty() || !running_.load(); });

            if(flushBuffers_.empty() && currentBuffer_.empty())
                continue;

            if(!currentBuffer_.empty())
                flushBuffers_.push_back(std::move(currentBuffer_));
        }

        // 写日志（锁外执行 I/O）
        for(auto& buf : flushBuffers_)
        {
            for(auto& entry : buf)
            {
                // 控制台输出
                std::ostream& os = (entry.first == LOG_ERROR) ? std::cerr : std::cout;
                os << entry.second << '\n';

                // 文件输出
                if(logFile_.is_open())
                {
                    logFile_ << entry.second << '\n';
                    currentFileSize_ += entry.second.size() + 1;

                    if(currentFileSize_ >= maxFileSize_)
                    {
                        RotateLog();
                    }
                }
            }
        }
        std::cout.flush();
        std::cerr.flush();
        if(logFile_.is_open())
            logFile_.flush();

        // 回收一个 spare buffer
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(nextBuffer_.empty())
            {
                nextBuffer_.swap(flushBuffers_.back());
                nextBuffer_.clear();
            }
        }
        flushBuffers_.clear();
    }
}
