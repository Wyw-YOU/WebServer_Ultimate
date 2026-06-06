#pragma once

#include "util/Error.hpp"
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

    bool AddFd(int fd, uint32_t events);

    bool ModFd(int fd, uint32_t events);

    bool DelFd(int fd);

    int Wait(int timeoutMs);

    epoll_event GetEvent(size_t index) const;

private:
    int epollFd_;
    std::vector<epoll_event> events_;
};