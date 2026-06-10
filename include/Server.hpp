#pragma once

#include "Log.hpp"
#include "net/Acceptor.hpp"
#include "net/InetAddress.hpp"
#include "net/Socket.hpp"
#include "net/Connection.hpp"
#include "buffer/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "net/Epoll.hpp"
#include "net/EventLoop.hpp"
#include "net/Channel.hpp"
#include "thread/ThreadPool.hpp"

#include <iostream>
#include <cstring>
#include <unordered_map>
#include <memory>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * @brief WebServer类
 */
class Server
{
public:
    /**
     * @brief 构造函数
     *
     * @param port 监听端口
     */
    Server(int port, const std::string& resourceDir);

    /**
     * @brief 启动服务器
     */
    void Start();

private:
    void HandleListenEvent();
    void HandleReadEvent(std::shared_ptr<Connection> conn);
    void HandleWriteEvent(std::shared_ptr<Connection> conn);
    void CloseConnection(std::shared_ptr<Connection> conn);

    void HandleFinished(std::shared_ptr<Connection> conn);

private:
    int port_;
    std::string resourceDir_;
    Acceptor acceptor_;
    EventLoop loop_;
    ThreadPool pool_;

    std::unique_ptr<Channel> listenChannel_;
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;
};