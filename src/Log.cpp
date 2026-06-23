#include "Log.hpp"
#include "AsyncLogger.hpp"

#include <iostream>

// 初始化静态成员：默认最低级别为 LOG_NORMAL（输出所有日志）
LogLevel Log::s_minLevel = LOG_NORMAL;

void Log::SetMinLevel(LogLevel level)
{
    s_minLevel = level;
}

LogLevel Log::GetMinLevel()
{
    return s_minLevel;
}

const char* Log::LevelName(LogLevel level)
{
    return LevelToString(level);
}

const char* Log::LevelToString(LogLevel level)
{
    // 按枚举顺序定义字符串数组
    static const char* const levelNames[] = {
        "NORMAL",
        "DEBUG",
        "ERROR"
    };
    // 确保枚举值在有效范围内
    if (level < LOG_NORMAL || level > LOG_ERROR)
        return "UNKNOWN";
    return levelNames[level];
}

void Log::Print(LogLevel level, const std::string& msg)
{
    if (level < s_minLevel)
        return;

    // AsyncLogger 已启动则走异步路径，否则同步输出
    AsyncLogger::Append(level, msg);
}