---
name: timer
description: Implement or modify the timer/timeout system. Use when working on connection timeouts, keep-alive timers, or scheduled tasks.
---

# Timer Skill

Guide for implementing connection timeout management.

## Purpose

- Close idle connections that exceed keep-alive timeout
- Prevent resource exhaustion from stale connections
- Support delayed/scheduled tasks

## Architecture: Min-Heap Timer

```
Min-Heap (sorted by expiration time)
┌─────────────────────────────────────┐
│  fd=3  expire=1000ms  ← top (soonest)│
│  fd=7  expire=1500ms                │
│  fd=12 expire=3000ms                │
│  fd=5  expire=5000ms                │
└─────────────────────────────────────┘
```

## Implementation

```cpp
#pragma once
#include <chrono>
#include <queue>
#include <unordered_map>
#include <functional>

struct TimerNode {
    int fd;
    std::chrono::steady_clock::time_point expire_time;
    std::function<void()> callback;

    bool operator>(const TimerNode& other) const {
        return expire_time > other.expire_time;
    }
};

class Timer {
public:
    using Clock = std::chrono::steady_clock;
    using Ms = std::chrono::milliseconds;

    // Add/update timer for a connection
    void Add(int fd, int timeout_ms, std::function<void()> cb);

    // Remove timer (connection closed normally)
    void Remove(int fd);

    // Get ms until next timer fires (for epoll_wait timeout)
    int GetNextTimeout() const;

    // Process all expired timers
    void Tick();

    size_t Size() const;

private:
    std::priority_queue<TimerNode, std::vector<TimerNode>, std::greater<>> heap_;
    std::unordered_map<int, bool> expired_;  // lazy deletion marker
};
```

## Integration with Event Loop

```cpp
while (running_) {
    int timeout = timer.GetNextTimeout();  // -1 if no timers
    int n = epoll_wait(epfd, events, MAX_EVENTS, timeout);

    // Process I/O events
    for (int i = 0; i < n; ++i) { /* handle events */ }

    // Process expired timers
    timer.Tick();
}
```

## Lazy Deletion

When a connection closes normally (not by timeout):
- Mark fd as deleted in `expired_` map
- When popping from heap, skip nodes marked as deleted
- This avoids O(n) removal from the heap

## Timeout Values

| Scenario           | Timeout   |
|--------------------|-----------|
| Keep-alive         | 60s       |
| Read (first byte)  | 10s       |
| Write (complete)   | 10s       |
| Graceful shutdown  | 5s        |

## Alternative: Timerfd

For more precise timing, use Linux `timerfd`:

```cpp
int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
struct itimerspec ts = { .it_interval = {0}, .it_value = {5, 0} };
timerfd_settime(timerfd, 0, &ts, nullptr);
// Add timerfd to epoll — read event means timeout fired
```

## Important

- Use `std::chrono::steady_clock` (not `system_clock`) for monotonic time
- Process all expired timers in one `Tick()` call
- Keep timeout values configurable (command-line or config file)
