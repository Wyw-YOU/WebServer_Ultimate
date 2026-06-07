#pragma

#include "net/Epoll.hpp"

class EventLoop
{
public:
    explicit EventLoop(int maxEvents = 1024);

    void Loop();

private:
    Epoller epoller_;
};