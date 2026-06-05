---
name: log
description: Implement or modify the logging system. Use when working on log levels, async logging, file output, or log formatting.
---

# Log Skill

Guide for enhancing the logging system.

## Current State

Basic logging exists in `include/Log.hpp` / `src/Log.cpp`:
- Two levels: DEBUG, ERROR
- Outputs to stdout only
- Not thread-safe
- Not used anywhere in the codebase

## Target: Async Thread-Safe Logger

### Log Levels (in order)

```
TRACE < DEBUG < INFO < WARN < ERROR < FATAL
```

### Design: Frontend/Backend Split

```
[Worker Threads]                    [Logger Thread]
 LOG_INFO("msg")                       │
      │                                │
      ▼                                │
 ┌─────────┐    mutex/spinlock    ┌─────────┐
 │  Buffer  │ ──────────────────► │  Buffer  │ ──► flush to file
 │  (ring)  │                     │  (ring)  │
 └─────────┘                     └─────────┘
   Frontend                         Backend
```

### Implementation

```cpp
#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <format>  // C++20

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

class Logger {
public:
    static Logger& Instance();

    void SetLevel(LogLevel level);
    void SetOutput(const std::string& filepath);  // file output
    void EnableConsole(bool enable);

    void Log(LogLevel level, const char* file, int line, const char* func,
             std::string_view msg);

private:
    Logger();
    ~Logger();

    LogLevel level_ = LogLevel::DEBUG;
    std::ofstream file_;
    std::mutex mutex_;
    bool console_ = true;
};

// Macros (capture file/line/function)
#define LOG_TRACE(msg) Logger::Instance().Log(LogLevel::TRACE, __FILE__, __LINE__, __func__, msg)
#define LOG_DEBUG(msg) Logger::Instance().Log(LogLevel::DEBUG, __FILE__, __LINE__, __func__, msg)
#define LOG_INFO(msg)  Logger::Instance().Log(LogLevel::INFO,  __FILE__, __LINE__, __func__, msg)
#define LOG_WARN(msg)  Logger::Instance().Log(LogLevel::WARN,  __FILE__, __LINE__, __func__, msg)
#define LOG_ERROR(msg) Logger::Instance().Log(LogLevel::ERROR, __FILE__, __LINE__, __func__, msg)
#define LOG_FATAL(msg) Logger::Instance().Log(LogLevel::FATAL, __FILE__, __LINE__, __func__, msg)
```

### Format

```
2024-01-15 14:30:45.123 [INFO ] Server.cpp:42 Start() — Server listening on port 8080
```

### Thread Safety

- Use `std::mutex` for initial implementation
- For high performance: lock-free ring buffer + dedicated flush thread
- Batch flush: collect N messages or every T ms, then flush once

### Log Rotation

```cpp
// Rotate when file exceeds size limit
if (file_.tellp() > max_size_) {
    file_.close();
    rename(current_path, archive_path);
    file_.open(current_path);
}
```

## Important

- Never log in the hot path without buffering (it kills performance)
- Use `std::string_view` to avoid string copies in log calls
- Flush on FATAL (before abort)
- Consider `std::format` (C++20) for type-safe formatting
