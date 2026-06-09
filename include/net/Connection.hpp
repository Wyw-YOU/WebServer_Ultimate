#pragma once

#include "buffer/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "util/FileUtil.hpp"
#include "thread/ThreadPool.hpp"
#include "net/Channel.hpp"
#include "Log.hpp"

#include <sys/socket.h>   // recv, send
#include <unistd.h>       // read, write, close
#include <atomic>
#include <iostream>
#include <string>

// 连接状态
enum class ConnState
{
    Connected,
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

    // 连接状态管理
    ConnState GetState() const;
    void SetState(ConnState state);

    int GetFd() const;
    Channel* GetChannel() const;

    bool Close();

    bool IsKeepAlive();

    // 设置回调
    void SetReadCallback(std::function<void()> cb);
    void SetWriteCallback(std::function<void()> cb);
    void SetCloseCallback(std::function<void()> cb);

    void EnableReading();
    void EnableWriting();
    void DisableWriting();

private:
    int fd_;
    // ConnState state_;
    std::atomic<ConnState> state_;
    std::string resourceDir_;

    std::unique_ptr<Channel> channel_;

    Buffer readBuffer_;
    Buffer writeBuffer_;
    HttpRequest request_;
    HttpResponse response_;
};