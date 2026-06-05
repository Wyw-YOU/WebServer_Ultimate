---
name: thread-pool
description: Implement or modify the thread pool. Use when working on concurrency, task queuing, or worker threads.
---

# Thread Pool Skill

Guide for implementing a fixed-size thread pool for handling connections.

## Architecture

```
Main Thread (epoll_wait)
     │
     ▼
┌─────────────────────────┐
│     Task Queue (FIFO)   │
│  ┌───┬───┬───┬───┬───┐  │
│  │ T1│ T2│ T3│ T4│...│  │
│  └───┴───┴───┴───┴───┘  │
└────────┬────────────────┘
         │
    ┌────┼────┬────┬────┐
    ▼    ▼    ▼    ▼    ▼
  [W1]  [W2]  [W3]  [W4]  ← Worker Threads
```

## Implementation

```cpp
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stop_token>  // C++20

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Submit a task and get a future
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    std::vector<std::jthread> workers_;          // C++20 jthread
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};
```

## Key Design Decisions

1. **Use `std::jthread`** (C++20): auto-joins on destruction, supports `std::stop_token`
2. **Fixed size**: create all threads in constructor, no dynamic scaling
3. **FIFO queue**: tasks executed in submission order
4. **Condition variable**: workers sleep when queue is empty, wake on new task

## Thread Safety

- Protect `tasks_` queue with `mutex_`
- Use `cv_.notify_one()` after pushing a task (not `notify_all`)
- Check `stop_` flag in worker loop to allow graceful shutdown

## Integration with Reactor

```
Main Thread:
  epoll_wait() → events[]
  for each event:
      handler->OnRead()   // parse request
      thread_pool.Submit([handler]() {
          handler->Process();  // compute response
          // signal main thread to register EPOLLOUT
      });
```

## Performance Tips

- Pool size = `std::thread::hardware_concurrency()` (usually CPU core count)
- Avoid lock contention: keep critical section small
- Use `std::move` for task submission to avoid copies
- Consider lock-free queue for high-throughput scenarios

## Common Pitfalls

- Forgetting to lock before accessing shared queue
- Using `std::thread` instead of `std::jthread` (must manually join)
- Submitting tasks after destructor starts (check `stop_` flag first)
- Not handling exceptions in worker tasks (wrap in try/catch)
