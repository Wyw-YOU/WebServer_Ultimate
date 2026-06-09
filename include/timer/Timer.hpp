#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <cstdint>

class Timer
{
public:
    Timer() = default;

    void Add(int fd, int timeoutMs, const std::function<void()>& cb);
    void Adjust(int fd, int timeoutMs);
    void Delete(int fd);

    void Tick();
    int GetNextTick();

private:
    struct TimerNode
    {
        int fd;
        uint64_t expire;
        std::function<void()> cb;
    };

private:
    void SiftUp(size_t i);
    void SiftDown(size_t i);

    void SwapNode(size_t i,size_t j);

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int,size_t> ref_;
};