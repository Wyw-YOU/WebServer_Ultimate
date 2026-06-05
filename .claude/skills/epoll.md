---
name: epoll
description: Implement or modify the epoll-based event loop (Reactor model). Use when working on I/O multiplexing, event handling, or the main loop.
---

# Epoll Skill

Guide for implementing the epoll-based Reactor event loop.

## Architecture: Reactor Model

```
                  ┌─────────────┐
                  │  epoll_wait  │
                  └──────┬──────┘
                         │ events[]
            ┌────────────┼────────────┐
            ▼            ▼            ▼
     ┌──────────┐ ┌──────────┐ ┌──────────┐
     │ onRead   │ │ onWrite  │ │ onClose  │
     │ callback │ │ callback │ │ callback │
     └──────────┘ └──────────┘ └──────────┘
```

## Key Components

### 1. Epoll Wrapper (`Epoll.hpp/cpp`)
```cpp
#include <sys/epoll.h>

class Epoll {
public:
    static constexpr int MAX_EVENTS = 1024;

    Epoll();
    ~Epoll();

    void AddFd(int fd, uint32_t events);
    void ModFd(int fd, uint32_t events);
    void DelFd(int fd);

    int Wait(struct epoll_event* events, int max_events, int timeout_ms);

private:
    int epoll_fd_;
};
```

### 2. Event Handler Interface
```cpp
class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual void OnRead() = 0;
    virtual void OnWrite() = 0;
    virtual void OnClose() = 0;
    virtual int GetFd() const = 0;
};
```

### 3. Main Event Loop
```cpp
void Server::Start() {
    epoll_event events[MAX_EVENTS];
    while (running_) {
        int n = epoll_.Wait(events, MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i) {
            auto* handler = static_cast<EventHandler*>(events[i].data.ptr);
            if (events[i].events & EPOLLIN)  handler->OnRead();
            if (events[i].events & EPOLLOUT) handler->OnWrite();
            if (events[i].events & (EPOLLERR | EPOLLHUP)) handler->OnClose();
        }
    }
}
```

## ET vs LT

- **LT (Level Triggered)**: Default. Notifies repeatedly while fd is ready. Simpler, safer.
- **ET (Edge Triggered)**: Notifies once per state change. Higher performance, but MUST read/write until `EAGAIN`.

This project uses **ET mode** (`EPOLLET`).

### ET Pattern
```cpp
// Must loop until EAGAIN in ET mode
while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // done
        perror("read");
        break;
    }
    if (n == 0) {
        // client closed
        break;
    }
    // process buf...
}
```

## Non-blocking Setup

```cpp
#include <fcntl.h>

void SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

## Important

- Always set the listen socket to non-blocking
- Always set client sockets to non-blocking
- Use `EPOLLONESHOT` for multi-threaded scenarios to avoid thundering herd
- Close fd in the handler's destructor or OnClose
