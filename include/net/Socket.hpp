#pragma once

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

/**
 * @brief Socket封装
 */
class Socket
{
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();

    int GetFd() const;
    void Bind(const sockaddr_in* addr);
    void Listen(int backlog = 128);
    int Accept(sockaddr_in* addr);
    void Close();

    // 设置非阻塞
    void SetNonBlocking();

private:
    int fd_;        // listenfd
};