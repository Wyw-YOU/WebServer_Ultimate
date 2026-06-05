#include "net/InetAddress.hpp"

#include <cstring>

// 构造
InetAddress::InetAddress()
{
    memset(&addr_,0,sizeof(addr_));
}

InetAddress::InetAddress(uint16_t port)
{
    memset(&addr_,0,sizeof(addr_));

    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port)
{
    memset(&addr_,0,sizeof(addr_));

    addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
    addr_.sin_port = htons(port);
}

// 获取地址
sockaddr_in* InetAddress::GetAddr()
{
    return &addr_;
}

const sockaddr_in* InetAddress::GetAddr() const
{
    return &addr_;
}