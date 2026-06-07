#include "net/Connection.hpp"

Connection::Connection(int fd, const std::string& resourceDir)
    : fd_(fd),
      resourceDir_(resourceDir)
    { }

//  构建Http请求并返回HTTP响应
bool Connection::Process()
{
    // 解析请求
    std::string raw = readBuffer_.RetrieveAll();
    LOG_DEBUG("Raw request:\n" + raw);

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
bool Connection::Read()
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
            return false;
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
                return false;
            }
        }
    }

    return true;
}

//  发送HTTP响应数据
bool Connection::Write()
{
    if(writeBuffer_.ReadableBytes() == 0)
    {
        // 没有数据需要发送
        return true;
    }

    const std::string& data = writeBuffer_.Peek();
    ssize_t n = send(fd_, data.c_str(), data.size(), 0);
    if(n > 0)
    {
        writeBuffer_.Retrieve(n);
        LOG_DEBUG("Sent " + std::to_string(n) + " bytes to fd=" + std::to_string(fd_));
        return writeBuffer_.Empty();
    }
    else if(n == 0)
    {
        return false;
    }
    else
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return false;
        }
        else
        {
            return false;
        }
    }
}

int Connection::GetFd() const
{
    return fd_;
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