#include "Server.hpp"
#include "Log.hpp"
#include "net/Acceptor.hpp"
#include "net/InetAddress.hpp"
#include "net/Socket.hpp"
#include "buffer/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

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
        int clientfd = acceptor.Accept(client);

        if(clientfd < 0)
        {
            continue;
        }

        // 接受客户端请求数据并写入Buffer
        char recvbuffer[4096];
        int n = recv(clientfd, recvbuffer, sizeof(recvbuffer), 0);
        Buffer buffer;
        buffer.Append(recvbuffer, n);

        // if(n > 0)
        // {
        //     std::cout << buffer << std::endl;
        // }

        // 构建Http请求并返回HTTP响应
        HttpRequest request;
        HttpResponse response;
        request.Parse(buffer.RetrieveAll());
        response.SetStatus(200, "OK");
        response.SetHeader("Content-Type", "text/plain");
        response.SetBody("Welcome To our Server!");

        std::string responseStr = response.ToString();
        send(clientfd, responseStr.c_str(), responseStr.size(), 0);
        close(clientfd);
    }
}