#include "util/PasswordUtil.hpp"
#include "Log.hpp"

#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cstring>

// 生成 16 字节随机盐，返回 32 位 hex 串
std::string PasswordUtil::GenerateSalt()
{
    unsigned char salt[16];

    // 从 /dev/urandom 读取随机字节
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if(!urandom)
    {
        LOG_ERROR("Failed to open /dev/urandom");
        throw std::runtime_error("Failed to generate salt");
    }

    urandom.read(reinterpret_cast<char*>(salt), sizeof(salt));
    if(!urandom)
    {
        LOG_ERROR("Failed to read from /dev/urandom");
        throw std::runtime_error("Failed to generate salt");
    }

    // 转换为 hex 字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for(int i = 0; i < 16; ++i)
    {
        oss << std::setw(2) << static_cast<int>(salt[i]);
    }

    return oss.str(); // 32 字符
}

// SHA-256(salt + password)，返回 "salt:hash" 格式
std::string PasswordUtil::HashPassword(const std::string& password,
                                        const std::string& salt)
{
    // 拼接 salt + password
    std::string data = salt + password;

    // 计算 SHA-256
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
           data.length(), hash);

    // 转换为 hex 字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        oss << std::setw(2) << static_cast<int>(hash[i]);
    }

    std::string hashHex = oss.str(); // 64 字符

    // 返回 "salt:hash" 格式（32 + 1 + 64 = 97 字符）
    return salt + ":" + hashHex;
}

// 验证密码：解析 stored 中的 salt，对 input 哈希后常量时间比较
bool PasswordUtil::VerifyPassword(const std::string& input,
                                   const std::string& stored)
{
    // 解析 stored 格式："salt:hash"
    size_t colonPos = stored.find(':');
    if(colonPos == std::string::npos || colonPos != 32)
    {
        LOG_ERROR("Invalid password hash format");
        return false;
    }

    std::string salt = stored.substr(0, 32);
    std::string expectedHash = stored.substr(33); // 跳过 ':'

    // 对输入密码进行哈希
    std::string inputHash = HashPassword(input, salt);
    std::string inputHashHex = inputHash.substr(33); // 只取 hash 部分

    // 常量时间比较（防止时序攻击）
    if(inputHashHex.length() != expectedHash.length())
    {
        return false;
    }

    return CRYPTO_memcmp(inputHashHex.c_str(),
                        expectedHash.c_str(),
                        expectedHash.length()) == 0;
}

// 便捷方法：生成完整的密码哈希（自动生成盐）
std::string PasswordUtil::HashNewPassword(const std::string& password)
{
    std::string salt = GenerateSalt();
    return HashPassword(password, salt);
}
