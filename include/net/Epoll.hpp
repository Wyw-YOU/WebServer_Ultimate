#pragma once

#include "util/Error.hpp"
#include "net/Channel.hpp"
#include "Log.hpp"

#include <unistd.h>
#include <vector>
#include <sys/epoll.h>

/**
 * @brief epoll封装
 */
class Epoller
{
public:
    explicit Epoller(int maxEvents = 1024);

    ~Epoller();

    bool AddChannel(Channel* channel);
    bool ModChannel(Channel* channel);
    bool DelChannel(Channel* channel);

    int Wait(int timeoutMs);

    epoll_event GetEvent(size_t index) const;

private:
    int epollFd_;
    std::vector<epoll_event> events_;
};