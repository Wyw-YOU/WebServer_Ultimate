#include "Log.hpp"

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
    // 级别过滤：低于最低级别的不输出
    if (level < s_minLevel)
        return;

    const char* levelStr = LevelToString(level);
    // ERROR 级别输出到 stderr，其余到 stdout
    std::ostream& os = (level == LOG_ERROR) ? std::cerr : std::cout;
    os << "[" << levelStr << "] " << msg << std::endl;
}