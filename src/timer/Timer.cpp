#include "timer/Timer.hpp"

using namespace std::chrono;

namespace
{
    // 获取当前系统启动后的毫秒数（单调时钟，不受系统时间调整影响）
    uint64_t GetMs()
    {
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }
}

// 添加一个定时器事件
void Timer::Add(int fd, int timeoutMs, const std::function<void()>& cb)
{
    size_t expire = GetMs() + timeoutMs;

    if(ref_.count(fd))
    {
        size_t idx = ref_[fd];

        heap_[idx].expire = expire;
        heap_[idx].cb = cb;

        SiftDown(idx);
        SiftUp(idx);

        return;
    }

    size_t idx = heap_.size();
    ref_[fd] = idx;
    heap_.push_back({ fd, expire, cb });

    SiftUp(idx);
}

// 调整已存在定时器的超时时间
void Timer::Adjust(int fd, int timeoutMs)
{
    auto it = ref_.find(fd);
    if(it == ref_.end())
    {
        return;
    }

    size_t idx = it->second;

    heap_[idx].expire = GetMs() + timeoutMs;

    SiftDown(idx);
    SiftUp(idx);
}

// 删除指定的定时器
void Timer::Delete(int fd)
{
    auto it = ref_.find(fd);
    if(it == ref_.end())
    {
        return;
    }

    size_t idx = it->second;
    size_t last = heap_.size() - 1;

    if(idx < last)
    {
        SwapNode(idx, last);

        heap_.pop_back();
        ref_.erase(fd);

        SiftDown(idx);
        SiftUp(idx);
    }
    else
    {
        heap_.pop_back();
        ref_.erase(fd);
    }
}

// 向上调整堆：从索引 i 开始，不断与父节点比较，若当前节点过期时间更小则交换
void Timer::SiftUp(size_t i)
{
    while(i > 0)
    {
        size_t parent = (i - 1) / 2;
        if(heap_[parent].expire <= heap_[i].expire)
        {
            break;
        }

        SwapNode(parent, i);
        i = parent;
    }
}
// 向下调整堆：从索引 i 开始，与左右子节点中较小的比较，若子节点更小则交换
void Timer::SiftDown(size_t i)
{
    size_t n = heap_.size();

    while(true)
    {
        size_t smallest = i;

        size_t left = i * 2 + 1;
        size_t right = i * 2 + 2;

        if(left < n &&
           heap_[left].expire < heap_[smallest].expire)
        {
            smallest = left;
        }

        if(right < n &&
           heap_[right].expire < heap_[smallest].expire)
        {
            smallest = right;
        }

        if(smallest == i)
        {
            break;
        }

        SwapNode(i, smallest);
        i = smallest;
    }
}

// 处理所有已到期的定时器，执行其回调函数
void Timer::Tick()
{
    if(heap_.empty())
    {
        return;
    }

    size_t now = GetMs();

    while(!heap_.empty())
    {
        const TimerNode node = heap_.front();

        if(node.expire > now)
        {
            break;
        }

        Delete(node.fd);

        if(node.cb)
        {
            node.cb();
        }
    }
}

// 获取距离下一个定时器超时的剩余毫秒数，并清理所有已到期的定时器
// 返回值：剩余毫秒数（不小于0），若没有定时器则返回 -1
int Timer::GetNextTick()
{
    Tick();

    if(heap_.empty())
    {
        return -1;
    }
    int res = static_cast<int>(heap_.front().expire - GetMs());

    return res > 0 ? res : 0;
}

void Timer::SwapNode(size_t i, size_t j)
{
    std::swap(heap_[i], heap_[j]);

    ref_[heap_[i].fd] = i;
    ref_[heap_[j].fd] = j;
}