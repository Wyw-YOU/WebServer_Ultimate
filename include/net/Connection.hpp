#pragma once

#include "buffer/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "util/FileUtil.hpp"
#include "thread/ThreadPool.hpp"
#include "net/Channel.hpp"
#include "net/EventLoop.hpp"
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

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(int fd, EventLoop* loop, const std::string& resourceDir);
    // ~Connection()
    // {
    //     LOG_ERROR(
    //         "Connection destroyed fd="
    //         + std::to_string(fd_));
    // }

    using ReadEventCallback = std::function<void(std::shared_ptr<Connection>)>;
    using WriteEventCallback = std::function<void(std::shared_ptr<Connection>)>;
    using ConnectionCloseCallback = std::function<void(std::shared_ptr<Connection>)>;
    
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

    // IO回调
    void SetReadCallback(std::function<void()> cb);
    void SetWriteCallback(std::function<void()> cb);
    void SetCloseCallback(std::function<void()> cb);

    // 业务回调
    void SetOnRead(ReadEventCallback cb);
    void SetOnWriteComplete(WriteEventCallback cb);
    void SetOnClose(ConnectionCloseCallback cb);

    // 事件修改
    void EnableReading();
    void EnableWriting();
    void DisableWriting();

    // 事件处理
    void HandleRead();
    void HandleWrite();
    void HandleClose();

    void UpdateChannel(Epoller& epoller);

private:
    int fd_;
    EventLoop* loop_;
    std::atomic<ConnState> state_;
    std::string resourceDir_;

    std::unique_ptr<Channel> channel_;

    Buffer readBuffer_;
    Buffer writeBuffer_;
    HttpRequest request_;
    HttpResponse response_;

    ReadEventCallback onRead_;
    WriteEventCallback onWrite_;
    ConnectionCloseCallback onClose_;
};