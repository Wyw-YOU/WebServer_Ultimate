#include "Server.hpp"
#include "Log.hpp"
#include "net/Acceptor.hpp"
#include "net/InetAddress.hpp"
#include "net/Socket.hpp"

#include <iostream>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

Server::Server(int port)
    : port_(port)
    { }

void Server::Start()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if(listenfd < 0)
    {
        LOG_DEBUG("socket error!");
        return;
    }
    Socket socket(listenfd);
    Acceptor acceptor(port_);
    // sockaddr_in addr{};

    // addr.sin_family = AF_INET;
    // addr.sin_addr.s_addr = INADDR_ANY;      // 监听所有网卡
    // addr.sin_port = htons(port_);

    // if(bind(listenfd, (sockaddr*)&addr, sizeof(addr)) < 0)
    // {
    //     perror("bind error!");
    //     return;
    // }

    // if(listen(listenfd, 128) < 0)
    // {
    //     perror("listen error");
    //     return;
    // }

    LOG_NORMAL("WebServer started on port " + std::to_string(port_));

    while(true)
    {
        InetAddress client;

        socklen_t len = sizeof(client);

        // 客户端请求 clientfd
        int connfd = acceptor.Accept(client);

        if(connfd < 0)
        {
            continue;
        }

        char buffer[4096];
        int n =recv(connfd, buffer, sizeof(buffer), 0);

        if(n > 0)
        {
            std::cout << buffer << std::endl;
        }

        const char* response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "\r\n"
            "Hello WebServer";

        send(connfd, response, strlen(response), 0);
        close(connfd);
    }
}