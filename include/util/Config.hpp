#pragma once

#include <cstdlib>
#include <string>
#include <cstring>

class Config
{
public:
    // 获取环境变量，不存在时返回 fallback
    static const char* Get(const char* key, const char* fallback = nullptr)
    {
        const char* value = std::getenv(key);
        return value ? value : fallback;
    }

    // 获取整数类型的环境变量
    static int GetInt(const char* key, int fallback = 0)
    {
        const char* value = std::getenv(key);
        if(!value)
            return fallback;

        try
        {
            return std::stoi(value);
        }
        catch(...)
        {
            return fallback;
        }
    }

    // 获取字符串类型的环境变量
    static std::string GetString(const char* key, const std::string& fallback = "")
    {
        const char* value = std::getenv(key);
        return value ? std::string(value) : fallback;
    }

    // 检查环境变量是否存在
    static bool Has(const char* key)
    {
        return std::getenv(key) != nullptr;
    }

    // 获取必需的环境变量，不存在时抛出异常
    static const char* Require(const char* key)
    {
        const char* value = std::getenv(key);
        if(!value)
        {
            throw std::runtime_error(
                std::string("Required environment variable not set: ") + key);
        }
        return value;
    }
};
