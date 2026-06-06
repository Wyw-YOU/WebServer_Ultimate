#pragma once

#include "Socket.hpp"
#include "InetAddress.hpp"

class Acceptor
{
public:
    explicit Acceptor(uint16_t port);

    int Accept(InetAddress& clientAddr);
    int GetFd() const;
    void SetNonBlocking();

private:
    Socket socket_;
};