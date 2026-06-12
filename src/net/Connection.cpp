#include "net/Connection.hpp"


Connection::Connection(int fd, EventLoop* loop, const std::string& resourceDir)
    : fd_(fd),
      loop_(loop),
      state_(ConnState::Connected),
      resourceDir_(resourceDir)
    {
        // channel_ = std::make_unique<Channel>(fd_);
        channel_ = std::unique_ptr<Channel>(new Channel(fd_));
    }

//  构建Http请求并返回HTTP响应
bool Connection::Process()
{
    // std::cout << "Process fd = " + std::to_string(fd_) << std::endl;
    // LOG_DEBUG("Process in thread id=" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    // 解析请求
    std::string raw = readBuffer_.RetrieveAll();
    // LOG_DEBUG("Raw request:\n" + raw);

    if(!request_.Parse(raw))
    {
        response_.SetStatus(400, "Bad Request");
        response_.SetBody("400 Bad Request");
    }
    else
    {
        // 根据路径路由
        std::string path = request_.Path();
        if(path == "/")
            path = "/hello";

        std::string filename = resourceDir_ + path + ".html";
        std::string html;

        if(FileUtil::ReadFile(filename, html))
        {
            response_.SetStatus(200, "OK");
            response_.SetHeader("Content-Type", "text/html");
            response_.SetHeader("Connection", request_.IsKeepAlive() ? "keep-alive" : "close");
            response_.SetBody(html);
        }
        else
        {
            response_.SetStatus(404, "Not Found");
            response_.SetBody("404 Not Found");
        }
    }

    // 将响应序列化写入发送缓冲区
    std::string resp = response_.ToString();
    writeBuffer_.Append(resp.c_str(), resp.size());

    return true;
}

//  接受客户端请求数据并写入Buffer
ReadResult Connection::Read()
{
    // 接受客户端请求数据并写入Buffer
    char recvbuffer[4096];

    // 循坏读取，直到没有数据可读（非阻塞套接字）
    while(true)
    {
        ssize_t n = recv(fd_, recvbuffer, sizeof(recvbuffer), 0);

        if(n > 0)
        {
            readBuffer_.Append(recvbuffer, n);
            LOG_DEBUG("Received " + std::to_string(n) + " bytes from fd=" + std::to_string(fd_));
        }
        else if(n == 0)
        {
            // 客户端关闭连接
            return ReadResult::Closed;
        }
        else
        {
            // 读取错误
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 没有数据可读了，正常情况
                break;
            }
            else
            {
                // 其他错误
                return ReadResult::Error;
            }
        }
    }

    return ReadResult::Success;
}

//  发送HTTP响应数据（循环发送，配合ET模式）
WriteResult Connection::Write()
{
    while(writeBuffer_.ReadableBytes() > 0)
    {
        auto readable = writeBuffer_.PeekReadable();
        ssize_t n = send(fd_, readable.first, readable.second, 0);

        if(n > 0)
        {
            writeBuffer_.Retrieve(n);
            LOG_DEBUG("Sent " + std::to_string(n) + " bytes to fd=" + std::to_string(fd_));
        }
        else if(n == 0)
        {
            return WRITE_COMPLETE;
        }
        else
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return WRITE_AGAIN;
            }
            else
            {
                return WRITE_ERROR;
            }
        }
    }
    return WRITE_COMPLETE;
}

int Connection::GetFd() const
{
    return fd_;
}
Channel* Connection::GetChannel() const
{
    return channel_.get();
}

// 关闭连接
bool Connection::Close()
{
    if(fd_ != -1)
    {
        close(fd_);
        fd_ = -1;
        return true;
    }
    return false;
}

// 判断是否是长连接
bool Connection::IsKeepAlive()
{
    return request_.IsKeepAlive();
}

ConnState Connection::GetState() const
{
    return state_.load();
}
void Connection::SetState(ConnState state)
{
    state_.store(state);
}

    // IO回调: Channel
void Connection::SetReadCallback(std::function<void()> cb)
{
    channel_->SetReadCallback(std::move(cb));
}
void Connection::SetWriteCallback(std::function<void()> cb)
{
    channel_->SetWriteCallback(std::move(cb));
}
void Connection::SetCloseCallback(std::function<void()> cb)
{
    channel_->SetCloseCallback(std::move(cb));
}

    // 事件修改
void Connection::EnableReading()
{
    channel_->SetEvents(EPOLLIN | EPOLLET);
}
void Connection::EnableWriting()
{
    channel_->SetEvents(EPOLLIN | EPOLLOUT | EPOLLET);
}
void Connection::DisableWriting()
{
    channel_->SetEvents(EPOLLIN | EPOLLET);
}

void Connection::ResetForNextRequest()
{
    request_.Reset();
    response_.Reset();
    writeBuffer_.RetrieveAll();
    
    SetState(ConnState::Connected);
    EnableReadEvent();

    LOG_DEBUG("Reset connection fd= " + std::to_string(fd_));
}

    // 事件处理
void Connection::HandleRead()
{
    LOG_DEBUG("HandleRead fd=" + std::to_string(fd_));

    auto result = Read();
    if(result == ReadResult::Closed)
    {
        if(onClose_)
        {
            onClose_(shared_from_this());
        }
        return;
    }

    if(result == ReadResult::Error)
    {
        if(onClose_)
        {
            onClose_(shared_from_this());
        }
        return;
    }

    if(onRead_)
    {
        onRead_(shared_from_this());
    }
}
void Connection::TrySend()
{
    WriteResult result = SendResponse();
    HandleWriteResult(result);
}
void Connection::HandleWrite()
{
    LOG_DEBUG("HandleWrite fd=" + std::to_string(fd_));
    TrySend();
}
void Connection::HandleClose()
{
    LOG_DEBUG("HandleClose fd=" + std::to_string(fd_));
    if(onClose_)
    {
        onClose_(shared_from_this());
    }
}
void Connection::UpdateChannel(Epoller& epoller)
{
    epoller.ModChannel(channel_.get());
}

    // 业务回调: Connection
void Connection::SetOnRead(ReadEventCallback cb)
{
    onRead_ = std::move(cb);
}
// void Connection::SetOnWrite(WriteEventCallback cb)
// {
//     onWrite_ = std::move(cb);
// }
void Connection::SetOnClose(ConnectionCloseCallback cb)
{
    onClose_ = std::move(cb);
}
// void Connection::SetOnWriteComplete(WriteCompleteCallback cb)
// {
//     onWriteComplete_ = std::move(cb);
// }



WriteResult Connection::SendResponse()
{
    return Write();
}

void Connection::EnableReadEvent()
{
    EnableReading();
    UpdateChannel(loop_->GetEpoller());
}
void Connection::EnableWriteEvent()
{
    EnableWriting();
    UpdateChannel(loop_->GetEpoller());
}

// 是否长连接？
bool Connection::OnResponseFinished()
{
    // std::cout << "keepalive = " << IsKeepAlive() << std::endl;
    if(IsKeepAlive())
    {
        ResetForNextRequest();
        return true;
    }

    SetState(ConnState::Closed);
    return false;
}

// 解析
void Connection::ProcessInWorker()
{
    Process();
    SetState(ConnState::Writing);

    auto self = shared_from_this();
    loop_->QueueInLoop(
    [self]()
    {
        if(self->GetState() == ConnState::Closed)
        {
            return;
        }

        self->TrySend();
    });
}


//--------------private:
void Connection::HandleWriteResult(WriteResult result)
{
    switch(result)
    {
        case WRITE_COMPLETE:
        {
            if(!OnResponseFinished())
            {
                if(onClose_)
                {
                    onClose_(shared_from_this());
                }
            }
            break;
        }
        case WRITE_AGAIN:
        {
            EnableWriteEvent();
            LOG_DEBUG("Waiting to send more data fd= " + std::to_string(fd_));
            break;
        }
        case WRITE_ERROR:
        {
            if(onClose_)
            {
                onClose_(shared_from_this());
            }
            break;
        }
    }
}