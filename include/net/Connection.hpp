#pragma once

#include "buffer/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "util/FileUtil.hpp"

#include <sys/socket.h>   // recv, send
#include <unistd.h>       // read, write, close

class Connection
{
public:
    explicit Connection(int fd);

    bool Read();

    bool Process();

    bool Write();

    int GetFd() const;

    bool Close();

private:
    int fd_;

    Buffer readBuffer_;
    Buffer writeBuffer_;
    HttpRequest request_;
    HttpResponse response_;
};