#pragma once

#include <string>

class PasswordUtil
{
public:
    // 生成 16 字节随机盐，返回 32 位 hex 串
    static std::string GenerateSalt();

    // SHA-256(salt + password)，返回 "salt:hash" 格式（97 字符）
    static std::string HashPassword(const std::string& password,
                                    const std::string& salt);

    // 验证密码：解析 stored 中的 salt，对 input 哈希后常量时间比较
    static bool VerifyPassword(const std::string& input,
                               const std::string& stored);

    // 便捷方法：生成完整的密码哈希（自动生成盐）
    static std::string HashNewPassword(const std::string& password);
};
