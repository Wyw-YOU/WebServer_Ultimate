#include "net/Connection.hpp"

Connection::Connection(int fd)
    : fd_(fd)
    { }

//  构建Http请求并返回HTTP响应
bool Connection::Process()
{
    // 构建Http请求
    std::string html;

    if(FileUtil::ReadFile("resources/hello.html", html))
    {
        response_.SetStatus(200, "OK");
        response_.SetHeader("Content-Type", "text/html");
        response_.SetBody(html);
    }
    else
    {
        response_.SetStatus(404, "Not Found");
        response_.SetBody("404 Not Found");
    }

    return true;
}

//  接受客户端请求数据并写入Buffer
bool Connection::Read()
{
    // 接受客户端请求数据并写入Buffer
    char recvbuffer[4096];
    int n = recv(fd_, recvbuffer, sizeof(recvbuffer), 0);
    if(n > 0)
    {
        readBuffer_.Append(recvbuffer, n);
        return true;
    }
    else if(n == 0)
    {
        // 客户端关闭连接
        return false;
    }
    else
    {
        // 读取错误
        if(errno == EAGAIN && errno == EWOULDBLOCK)
        {
            // 没有数据可读了，正常情况
            return true;
        }
        else
        {
            // 其他错误
            return false;
        }
    }
}

//  发送HTTP响应数据
bool Connection::Write()
{
    if(writeBuffer_.ReadableBytes() == 0)
    {
        // 没有数据需要发送
        return true;
    }

    const std::string& data = writeBuffer_.RetrieveAll();
    ssize_t n = send(fd_, data.c_str(), data.size(), 0);
    if(n >= 0)
    {
        writeBuffer_.Retrieve(n); // 从缓冲区中移除已发送的数据
        return writeBuffer_.Empty(); // 如果缓冲区已空，返回true
    }
    else
    {
        if(errno == EAGAIN && errno == EWOULDBLOCK)
        {
            // 发送缓冲区满了，等待下一次可写事件
            return false;
        }
        else
        {
            // 其他错误
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