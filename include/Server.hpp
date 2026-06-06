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
    explicit Server(int port);

    /**
     * @brief 启动服务器
     */
    void Start();

private:
    void HandleListenEvent();
    void HandleReadEvent(int fd);
    void HandleWriteEvent(int fd);
    void CloseConnection(int fd);

private:
    int port_;
    Acceptor acceptor_;
    Epoller epoller_;

    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
};