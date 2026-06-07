#pragma once

#include "buffer/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "util/FileUtil.hpp"
#include "Log.hpp"

#include <sys/socket.h>   // recv, send
#include <unistd.h>       // read, write, close
#include <iostream>
#include <string>

enum ConnState
{
    Connected,
    READING,
    WRITING,
    CLOSED
};

class Connection
{
public:
    Connection(int fd, const std::string& resourceDir);

    bool Read();

    bool Process();

    bool Write();

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