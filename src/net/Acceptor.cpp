#include "net/Acceptor.hpp"
#include "net/InetAddress.hpp"
#include "net/Socket.hpp"
#include "Log.hpp"

// 构造: 绑定端口并监听
Acceptor::Acceptor(uint16_t port)
{
    InetAddress addr(port);

    socket_.Bind(addr.GetAddr());
    LOG_NORMAL("Bind to port " + std::to_string(port) + " success!");

    socket_.Listen();
    LOG_NORMAL("Listening on port " + std::to_string(port) + "...");
}

// 调用Socket的Accept方法，获取客户端连接
int Acceptor::Accept(InetAddress& clientAddr)
{
    return socket_.Accept(clientAddr.GetAddr());
}