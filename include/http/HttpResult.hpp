#pragma once
#include <string>

struct HttpResult
{
    int fd;

    std::string response;
    bool keepAlive = false;
    bool close = false;
};


// （Worker → IO）
// 职责:
// worker 输出结果
// IO线程唯一消费者（send / close）
// 不允许访问 Connection