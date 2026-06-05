#pragma once

#include <string>

/**
 * @brief 日志级别（普通 enum，兼容 C++98）
 */
enum LogLevel
{
    LOG_NORMAL,   // 普通信息
    LOG_DEBUG,    // 调试信息
    LOG_ERROR     // 错误信息
};

/**
 * @brief 简易日志类
 */
class Log
{
public:
    /**
     * @brief 输出日志
     * @param level 日志等级
     * @param msg   日志内容
     */
    static void Print(LogLevel level, const std::string& msg);

    /**
     * @brief 设置最低输出级别（低于此级别的日志不会输出）
     * @param level 最低级别
     */
    static void SetMinLevel(LogLevel level);

    /**
     * @brief 获取当前最低输出级别
     */
    static LogLevel GetMinLevel();

private:
    static LogLevel s_minLevel;   // 最低输出级别

    /**
     * @brief 将日志等级转换为字符串
     */
    static const char* LevelToString(LogLevel level);
};

// 便捷宏（注意：不再需要 LogLevel:: 前缀）
#define LOG_NORMAL(msg) Log::Print(LOG_NORMAL, msg)
#define LOG_DEBUG(msg)  Log::Print(LOG_DEBUG, msg)
#define LOG_ERROR(msg)  Log::Print(LOG_ERROR, msg)