#pragma once

#include <string>

// ==================== 日志开关配置 ====================
// 压测时取消下面一行的注释，或通过编译选项 -DDISABLE_ALL_LOG 定义
// #define DISABLE_ALL_LOG

// 日常调试时，如需开启 DEBUG 日志，取消下面一行的注释
// #define ENABLE_DEBUG_LOG
// ====================================================

/**
 * @brief 日志级别（枚举值与宏名相同，但通过内联函数避免命名冲突）
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
    static void Print(LogLevel level, const std::string& msg);
    static void SetMinLevel(LogLevel level);
    static LogLevel GetMinLevel();

    // 内联包装函数：避免宏与枚举值重名导致递归展开
    static inline void Normal(const std::string& msg) { Print(LOG_NORMAL, msg); }
    static inline void Debug(const std::string& msg)  { Print(LOG_DEBUG, msg); }
    static inline void Error(const std::string& msg)  { Print(LOG_ERROR, msg); }

private:
    static LogLevel s_minLevel;
    static const char* LevelToString(LogLevel level);
};

// ==================== 宏定义（只定义一次，根据开关选择不同行为） ====================
#ifdef DISABLE_ALL_LOG
    // 压测模式：所有日志完全消除（零开销）
    #define LOG_NORMAL(msg) ((void)0)
    #define LOG_DEBUG(msg)  ((void)0)
    #define LOG_ERROR(msg)  ((void)0)
#else
    // 正常模式：NORMAL 和 ERROR 始终输出，DEBUG 由 ENABLE_DEBUG_LOG 控制
    #define LOG_NORMAL(msg) Log::Normal(msg)
    #define LOG_ERROR(msg)  Log::Error(msg)

    #ifdef ENABLE_DEBUG_LOG
        #define LOG_DEBUG(msg) Log::Debug(msg)
    #else
        #define LOG_DEBUG(msg) ((void)0)
    #endif
#endif
// ====================================================================