#pragma once

#include <arpa/inet.h>
#include <string>

/**
 * @brief IP地址封装
 */
class InetAddress
{
public:
    InetAddress();
    explicit InetAddress(uint16_t port);

    InetAddress(const std::string& ip, uint16_t port);

    sockaddr_in* GetAddr();
    const sockaddr_in* GetAddr() const;
    std::string ToIp() const;
    uint16_t ToPort() const;

private:
    sockaddr_in addr_;
};