#include "net/EventLoop.hpp"

EventLoop::EventLoop(int maxEvents)
    : epoller_(maxEvents)
    { }

Epoller& EventLoop::GetEpoller()
{
    return epoller_;
}

void EventLoop::Loop()
{
    while(true)
    {
        int eventCnt = epoller_.Wait(-1);
        if(eventCnt < 0)
        {
            LOG_ERROR("epoll_wait failed");
            continue;
        }

        for(int i = 0; i < eventCnt; ++i)
        {
            epoll_event ev = epoller_.GetEvent(i);
            Channel* channel = static_cast<Channel*>(ev.data.ptr);
            if(channel == nullptr)
                continue;

            channel->SetRevents(ev.events);

            channel->HandleEvent();
        }
    }
}