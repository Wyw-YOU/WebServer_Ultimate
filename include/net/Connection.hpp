#pragma once

#include "buffer/Buffer.hpp"
#include "http/HttpResponse.hpp"
#include "http/HttpContext.hpp"
#include "http/MimeType.hpp"
#include "http/Router.hpp"
#include "util/FileUtil.hpp"
#include "util/GzipUtil.hpp"
#include "net/Channel.hpp"
#include "net/EventLoop.hpp"
#include "Log.hpp"

#include <sys/socket.h>   // recv, send
#include <sys/sendfile.h> // sendfile
#include <unistd.h>       // read, write, close
#include <atomic>
#include <iostream>
#include <string>
#include <memory>


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

// 读状态分析
enum class ReadResult
{
    Success,
    Closed,
    Error
};

// 解析状态
enum class ProcessResult
{
    Complete,
    Incomplete,
    Error
};

enum class WriteState
{
    Idle,       // 没有数据，不监听EPOLLOUT
    Pending,    // 有数据，等待写
    Writing     // 正在写
};

class EventLoop;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(int fd, EventLoop* loop, const std::string& resourceDir, Router* router);

    // 复用：重置所有状态以服务新连接
    void Reuse(int fd, EventLoop* loop, const std::string& resourceDir, Router* router);

    ReadResult Read();
    WriteResult Write();
    ProcessResult Process();

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

    // 事件修改
    void EnableReading();

    void ResetForNextRequest();     //继续监听

    // 事件处理
    void HandleRead();
    void TrySend();
    void HandleWrite();
    void HandleClose();

    void UpdateChannel(Epoller& epoller);
    WriteResult SendResponse();

    // 可读/写
    void EnableReadEvent();
    void EnableWriteEvent();
    void DisableWriteEvent();

    // 判断长连接
    bool OnResponseFinished();

    // 解析处理（IO线程内直接执行）
    void ProcessInWorker();
    bool HasPendingRequest() const;

private:
    void HandleWriteResult(WriteResult result);

private:
    int fd_;
    EventLoop* loop_;
    std::atomic<ConnState> state_;
    std::string resourceDir_;
    Router* router_;

    std::unique_ptr<Channel> channel_;

    Buffer readBuffer_;
    Buffer writeBuffer_;
    HttpResponse response_;
    HttpContext context_;

    WriteState writeState_ = WriteState::Idle;

    int sendFileFd_ = -1;
    off_t sendFileLen_ = 0;
};