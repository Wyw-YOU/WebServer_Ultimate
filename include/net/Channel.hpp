#pragma once

#include <functional>
#include <cstdint>
#include <sys/epoll.h>

class Channel
{
public:
    using EventCallback = std::function<void()>;

public:
    explicit Channel(int fd);
    ~Channel() = default;

    void HandleEvent();

public:
    // 设置IO回调
    void SetReadCallback(EventCallback cb);
    void SetWriteCallback(EventCallback cb);
    void SetCloseCallback(EventCallback cb);

public:
    int GetFd() const;

    // 其中Events为主动监听，Revents为返回事件属于被动监听
    uint32_t GetEvents() const;
    uint32_t GetRevents() const;

    void SetEvents(uint32_t events);
    void SetRevents(uint32_t revents);

private:
    int fd_;

    uint32_t events_;
    uint32_t revents_;

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
};