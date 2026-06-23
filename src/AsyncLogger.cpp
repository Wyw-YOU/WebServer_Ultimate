#include "AsyncLogger.hpp"
#include "Log.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>

AsyncLogger* AsyncLogger::s_instance = nullptr;

AsyncLogger::AsyncLogger() = default;
AsyncLogger::~AsyncLogger() = default;

void AsyncLogger::Init()
{
    if(s_instance)
        return;

    s_instance = new AsyncLogger();
    s_instance->currentBuffer_.reserve(kBufferThreshold);
    s_instance->nextBuffer_.reserve(kBufferThreshold);
    s_instance->running_.store(true);
    s_instance->thread_ = std::thread(&AsyncLogger::ThreadFunc, s_instance);
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
        }
    };

    for(auto& buf : s_instance->flushBuffers_)
        flush(buf);
    flush(s_instance->currentBuffer_);
    std::cout.flush();
    std::cerr.flush();

    delete s_instance;
    s_instance = nullptr;
}

void AsyncLogger::Append(LogLevel level, const std::string& msg)
{
    if(!s_instance)
    {
        // Logger 未启动，同步输出兜底
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
                std::ostream& os = (entry.first == LOG_ERROR) ? std::cerr : std::cout;
                os << entry.second << '\n';
            }
        }
        std::cout.flush();
        std::cerr.flush();

        // 回收一个 spare buffer，其余释放
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(nextBuffer_.empty())
            {
                // 回收一个已写完的 buffer 作为 spare
                nextBuffer_.swap(flushBuffers_.back());
                nextBuffer_.clear();
            }
        }
        flushBuffers_.clear();
    }
}
