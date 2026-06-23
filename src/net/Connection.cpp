#include "net/Connection.hpp"

#include <fcntl.h>
#include <sys/stat.h>


Connection::Connection(int fd, EventLoop* loop, const std::string& resourceDir, Router* router)
    : fd_(fd),
      loop_(loop),
      state_(ConnState::Connected),
      resourceDir_(resourceDir),
      router_(router)
    {
        // channel_ = std::make_unique<Channel>(fd_);
        channel_ = std::unique_ptr<Channel>(new Channel(fd_, true));
    }

//  构建Http请求并返回HTTP响应
ProcessResult Connection::Process()
{
    // LOG_DEBUG("Process in thread id=" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    // LOG_DEBUG("Raw request:\n" + raw);
    // 解析请求
    auto result = context_.ParseRequest(readBuffer_);
    switch(result)
    {
        case ParseResult::Incomplete:
            return ProcessResult::Incomplete;
    
        case ParseResult::Error:
        {
            response_.SetStatus(400, "Bad Request");
            response_.SetText("400 Bad Request");
            response_.SetKeepAlive(false);
            return ProcessResult::Error;
        }
    
        case ParseResult::Complete:
            break;
    }
    HttpRequest& request = context_.Request();

    std::string acceptEncoding = request.GetHeader("Accept-Encoding");
    bool clientGzip = (acceptEncoding.find("gzip") != std::string::npos);

    if(router_)
    {
        if(router_->Route(request, response_))
        {
            response_.SetKeepAlive(request.IsKeepAlive());

            if(clientGzip && GzipUtil::ShouldCompress(response_.GetHeader("Content-Type"))
               && !response_.GetBody().empty())
            {
                std::string compressed;
                if(GzipUtil::Compress(response_.GetBody(), compressed))
                {
                    response_.SetBody(compressed);
                    response_.SetHeader("Content-Encoding", "gzip");
                }
            }

            std::string resp = response_.ToString();
            writeBuffer_.Append(resp.c_str(), resp.size());

            return ProcessResult::Complete;
        }
    }

    // 根据路径路由
    std::string path = request.Path();
    // 拒绝非法路径 例如：GET /../../../etc/passwd
    if(path.find("..") != std::string::npos)
    {
        response_.SetStatus(403, "Forbidden");
        response_.SetText("403 Forbidden");
        response_.SetKeepAlive(false);
        
        std::string resp = response_.ToString();
        writeBuffer_.Append(resp.c_str(), resp.size());
        
        return ProcessResult::Complete;
    }

    if(path == "/")
        path = "/index.html";

    std::string filename = resourceDir_ + path;
    std::string fileContent;
    // 文件大小保护
    size_t fileSize = FileUtil::FileSize(filename);
    if(fileSize > 10 * 1024 * 1024)
    {
        response_.SetStatus(413, "Payload Too Large");
        response_.SetText("File Too Large");
        response_.SetKeepAlive(false);
    
        std::string resp = response_.ToString();
        writeBuffer_.Append(resp.c_str(), resp.size());
    
        return ProcessResult::Complete;
    }

    // sendfile 零拷贝：静态文件直接用 sendfile 传输 body
    // gzip 压缩时不能用 sendfile（压缩后数据与磁盘文件不同）

    if(request.Method() != "HEAD" && FileUtil::Exists(filename) && FileUtil::IsRegularFile(filename))
    {
        std::string mime = MimeType::GetMime(path);

        // gzip 路径：读文件到内存，后续统一压缩
        if(clientGzip && GzipUtil::ShouldCompress(mime))
        {
            if(FileUtil::ReadFile(filename, fileContent))
            {
                response_.SetStatus(200, "OK");
                response_.SetBody(fileContent);
                response_.SetHeader("Content-Type", mime);
            }
            else
            {
                response_.SetStatus(404, "Not Found");
                response_.SetText("404 Not Found");
            }
        }
        // sendfile 零拷贝路径
        else
        {
            int fileFd = open(filename.c_str(), O_RDONLY);
            if(fileFd >= 0)
            {
                struct stat st;
                fstat(fileFd, &st);

                response_.SetStatus(200, "OK");
                response_.SetHeader("Content-Type", mime);
                response_.SetHeader("Content-Length", std::to_string(st.st_size));
                response_.SetKeepAlive(request.IsKeepAlive());

                std::string headers = response_.HeadersOnly();
                writeBuffer_.Append(headers.c_str(), headers.size());

                sendFileFd_ = fileFd;
                sendFileLen_ = st.st_size;
                return ProcessResult::Complete;
            }
        }
    }

    // 非 sendfile 路径（HEAD / 404 / 文件打开失败）
    if(FileUtil::ReadFile(filename, fileContent))
    {
        response_.SetStatus(200, "OK");
        response_.SetBody(fileContent);
        response_.SetHeader("Content-Type", MimeType::GetMime(path));
    }
    else
    {
        std::string errorPage;
        if(FileUtil::ReadFile(resourceDir_ + "/404.html", errorPage))
        {
            response_.SetStatus(404, "Not Found");
            response_.SetBody(errorPage);
            response_.SetHeader("Content-Type", "text/html");
        }
        else
        {
            response_.SetStatus(404, "Not Found");
            response_.SetText("404 Not Found");
        }
    }
    response_.SetKeepAlive(request.IsKeepAlive());
    if(request.Method() == "HEAD")
    {
        response_.ClearBody();
    }

    // gzip 压缩
    if(clientGzip && GzipUtil::ShouldCompress(response_.GetHeader("Content-Type"))
       && !response_.GetBody().empty())
    {
        std::string compressed;
        if(GzipUtil::Compress(response_.GetBody(), compressed))
        {
            response_.SetBody(compressed);
            response_.SetHeader("Content-Encoding", "gzip");
        }
    }

    // 将响应序列化写入发送缓冲区
    std::string resp = response_.ToString();
    writeBuffer_.Append(resp.c_str(), resp.size());

    return ProcessResult::Complete;
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
    if(sendFileFd_ >= 0)
    {
        close(sendFileFd_);
        sendFileFd_ = -1;
    }
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
    return context_.Request().IsKeepAlive();
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
    channel_->EnableReading();
}

void Connection::ResetForNextRequest()
{
    context_.Reset();
    response_.Reset();
    writeBuffer_.RetrieveAll();

    if(sendFileFd_ >= 0)
    {
        close(sendFileFd_);
        sendFileFd_ = -1;
        sendFileLen_ = 0;
    }

    SetState(ConnState::Connected);
    EnableReadEvent();

    LOG_DEBUG("Reset connection fd= " + std::to_string(fd_));
}

    // 事件处理
void Connection::HandleRead()
{
    LOG_DEBUG("HandleRead fd=" + std::to_string(fd_));

    auto result = Read();
    if(result == ReadResult::Closed || result == ReadResult::Error)
    {
        HandleClose();
        return;
    }

    // 读到数据后直接在 IO 线程处理
    if(state_ == ConnState::Connected)
    {
        SetState(ConnState::Processing);
        loop_->AdjustTimer(fd_, 60000);
        ProcessInWorker();
    }
}
void Connection::TrySend()
{
    WriteResult result = SendResponse();
    HandleWriteResult(result);
}

void Connection::HandleWrite()
{
    writeState_ = WriteState::Writing;

    WriteResult result = SendResponse();

    if(writeBuffer_.ReadableBytes() == 0)
    {
        // buffer 排空，检查是否需要 sendfile
        if(sendFileFd_ >= 0)
        {
            off_t offset = 0;
            while(sendFileLen_ > 0)
            {
                ssize_t n = ::sendfile(fd_, sendFileFd_, &offset, sendFileLen_);
                if(n > 0)
                {
                    sendFileLen_ -= n;
                }
                else if(n == -1)
                {
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        EnableWriteEvent();
                        writeState_ = WriteState::Pending;
                        return;
                    }
                    close(sendFileFd_);
                    sendFileFd_ = -1;
                    HandleWriteResult(WRITE_ERROR);
                    return;
                }
            }
            close(sendFileFd_);
            sendFileFd_ = -1;
        }

        writeState_ = WriteState::Idle;
        DisableWriteEvent();
        HandleWriteResult(WRITE_COMPLETE);
        return;
    }

    // 没写完
    writeState_ = WriteState::Pending;
    EnableWriteEvent();

    HandleWriteResult(result);
}
void Connection::HandleClose()
{
    LOG_DEBUG("HandleClose fd=" + std::to_string(fd_));

    auto self = shared_from_this();
    loop_->QueueInLoop([self]()
    {
        self->loop_->RemoveConnection(self->fd_);
    });
}
void Connection::UpdateChannel(Epoller& epoller)
{
    epoller.ModChannel(channel_.get());
}

    // 业务回调已移除，业务处理在 HandleRead 中直接执行


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
    channel_->EnableWriting();
    UpdateChannel(loop_->GetEpoller());
}
void Connection::DisableWriteEvent()
{
    channel_->DisableWriting();
    UpdateChannel(loop_->GetEpoller());
}

// 响应是否结束：
bool Connection::OnResponseFinished()
{
    if(IsKeepAlive())
    {
        ResetForNextRequest();

        if(HasPendingRequest())
        {
            auto self = shared_from_this();

            loop_->QueueInLoop(
            [self]()
            {
                self->ProcessInWorker();
            });
        }

        return true;
    }

    SetState(ConnState::Closed);
    return false;
}

// 解析
void Connection::ProcessInWorker()
{
    ProcessResult result = Process();

    if(result == ProcessResult::Incomplete)
    {
        SetState(ConnState::Connected);
        return;
    }
    
    if(result == ProcessResult::Error)
    {
        SetState(ConnState::Closed);
    
        auto self = shared_from_this();
    
        loop_->QueueInLoop(
        [self]()
        {
            self->HandleClose();
        });
    
        return;
    }
    
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

bool Connection::HasPendingRequest() const
{
    return readBuffer_.ReadableBytes() > 0;
}

//--------------private:
void Connection::HandleWriteResult(WriteResult result)
{
    switch(result)
    {
        case WRITE_COMPLETE:
        {
            OnResponseFinished();
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
            HandleClose();
            break;
        }
    }
}