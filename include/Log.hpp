#pragma once

#include <string>

/**
 * @brief 日志级别
 */
enum class LogLevel
{
    DEBUG,
    ERROR
};

/**
 * @brief 简易日志类
 */
class Log
{
public:

    /**
     * @brief 输出日志
     *
     * @brief level 日志等级
     * @param msg 日志内容
     */
    static void Print(LogLevel level, const std::string& msg);

private:

    /**
     * @brief 获取日志等级字符串
     *
     * @param level 日志等级
     * @return std::string
     */
    static std::string LevelToString(LogLevel level);
};

#define LOG_DEBUG(msg) \
    Log::Print(LogLevel::DEBUG, msg)

#define LOG_ERROR(msg) \
    Log::Print(LogLevel::ERROR, msg)