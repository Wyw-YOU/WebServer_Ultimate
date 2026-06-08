#pragma once

#include "net/Epoll.hpp"
#include "net/Channel.hpp"

class EventLoop
{
public:
    explicit EventLoop(int maxEvents = 1024);

    void Loop();

    Epoller& GetEpoller();

private:
    Epoller epoller_;
};