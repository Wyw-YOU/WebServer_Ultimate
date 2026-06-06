#pragma once

#include "net/Epoll.hpp"
#include "util/Error.hpp"
#include "Log.hpp"

#include <cstring>

class Error
{
public:
    static void SysError(const std::string& msg)
    {
        LOG_ERROR(msg + ": " + strerror(errno));
    }
};