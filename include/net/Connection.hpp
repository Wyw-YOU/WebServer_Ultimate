#pragma once

#include "buffer/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "util/FileUtil.hpp"
#include "thread/ThreadPool.hpp"
#include "Log.hpp"

#include <sys/socket.h>   // recv, send
#include <unistd.h>       // read, write, close
#include <iostream>
#include <string>

// 连接状态
enum class ConnState
{
    Reading,
    Processing,
    Writing,
    Closed
};

// 写状态分析
enum WriteResult
{
    WRITE_COMPLETE,
    WRITE_AGAIN,
    WRITE_ERROR
};

class Connection
{
public:
    Connection(int fd, const std::string& resourceDir);
    
    bool Process();

    bool Read();
    WriteResult Write();

    int GetFd() const;

    bool Close();

    bool IsKeepAlive();

private:
    int fd_;
    ConnState state_;
    std::string resourceDir_;

    Buffer readBuffer_;
    Buffer writeBuffer_;
    HttpRequest request_;
    HttpResponse response_;
};