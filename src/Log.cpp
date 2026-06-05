#include "Log.hpp"

#include <iostream>

std::string Log::LevelToString(LogLevel level)
{
    switch(level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}

// 输出日志信息
void Log::Print(LogLevel level, const std::string& msg)
{
    std::cout << "[" << LevelToString(level) << "] " << msg << std::endl;
}