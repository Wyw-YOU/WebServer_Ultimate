# WebServer 框架设计文档

## 一、项目概览

| 项目       | 说明                                      |
| ---------- | ----------------------------------------- |
| 名称       | WebServer_Ultimate                        |
| 语言       | C++20                                     |
| 构建       | CMake 3.10+                               |
| 平台       | Linux (CentOS / Ubuntu)                   |
| 目标       | 高性能 HTTP 服务器，支持万级并发连接       |

### 核心特性清单

- [x] 基础 TCP 服务器（已完成）
- [ ] Reactor 事件驱动模型（ET 模式）
- [ ] epoll I/O 多路复用
- [ ] 非阻塞 I/O
- [ ] HTTP/1.1 协议解析
- [ ] 静态文件服务
- [ ] 线程池
- [ ] 定时器（连接超时管理）
- [ ] 异步日志系统
- [ ] 数据库连接池（MySQL）
- [ ] 压力测试
- [ ] 性能分析与优化

---

## 二、整体架构

### 2.1 Reactor 模型

```
                         ┌─────────────────────────────────────────────┐
                         │              Main Thread (Reactor)          │
                         │                                             │
                         │    ┌───────────────────────────────┐        │
                         │    │        epoll_wait (ET)        │        │
                         │    └──────────────┬────────────────┘        │
                         │                   │ events[]                │
                         │    ┌──────────────┼──────────────┐          │
                         │    ▼              ▼              ▼          │
                         │ ┌──────┐    ┌──────────┐   ┌──────────┐    │
                         │ │Accept│    │ onRead() │   │ onWrite()│    │
                         │ └──┬───┘    └────┬─────┘   └────┬─────┘    │
                         │    │             │              │           │
                         │    ▼             ▼              │           │
                         │ ┌──────────────────────┐        │           │
                         │ │   Thread Pool        │        │           │
                         │ │ ┌────┬────┬────┬────┐│        │           │
                         │ │ │ W1 │ W2 │ W3 │ W4 ││◄───────┘           │
                         │ │ └────┴────┴────┴────┘│                    │
                         │ └──────────────────────┘                    │
                         │                                             │
                         │    ┌──────────────────────┐                 │
                         │    │   Timer Manager      │                 │
                         │    │   (min-heap)         │                 │
                         │    └──────────────────────┘                 │
                         └─────────────────────────────────────────────┘
```

### 2.2 请求处理流程

```
客户端请求
    │
    ▼
epoll_wait() 检测到 EPOLLIN 事件
    │
    ▼
HttpConn::OnRead()
    ├── 从 fd 读取数据到读缓冲区
    ├── HttpParser 解析请求（状态机）
    │       REQUEST_LINE → HEADERS → BODY → COMPLETE
    └── 解析完成 → 提交到线程池
            │
            ▼
      ThreadPool::Submit()
            │
            ▼
      Worker Thread 执行：
        ├── Router 路由匹配
        ├── Handler 处理请求
        ├── 生成 HttpResponse
        └── 写入写缓冲区，注册 EPOLLOUT
            │
            ▼
      epoll_wait() 检测到 EPOLLOUT 事件
            │
            ▼
      HttpConn::OnWrite()
        ├── 发送写缓冲区数据
        ├── 发送完成 → 重置状态
        ├── Keep-Alive → 注册 EPOLLIN，等待下一个请求
        └── Close → 关闭连接，移除定时器
```

---

## 三、模块依赖关系

```
                    ┌──────────┐
                    │  main.cpp│
                    └────┬─────┘
                         │
                         ▼
                    ┌──────────┐
                    │  Server  │
                    └────┬─────┘
                         │
          ┌──────────────┼──────────────┐──────────────┐
          ▼              ▼              ▼              ▼
     ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐
     │  Epoll  │   │HttpConn │   │Timer    │   │Log      │
     └─────────┘   └────┬────┘   └─────────┘   └─────────┘
                        │
            ┌───────────┼───────────┐
            ▼           ▼           ▼
      ┌──────────┐ ┌──────────┐ ┌──────────┐
      │HttpParser│ │HttpRouter│ │HttpResponse│
      └──────────┘ └──────────┘ └──────────┘

      ┌──────────┐ ┌──────────┐
      │ThreadPool│ │ConnectionPool│
      └──────────┘ └──────────┘
```

### 依赖规则

1. **底层模块**：Log、Timer — 无内部依赖，被所有模块使用
2. **中间层模块**：Epoll、ThreadPool、ConnectionPool — 依赖 Log
3. **上层模块**：HttpParser、HttpRouter、HttpConn — 依赖底层 + 中间层
4. **顶层模块**：Server — 组装所有模块，依赖一切

---

## 四、目标文件结构

```
WebServer_Ultimate/
├── CMakeLists.txt              # 构建配置
├── README.md                   # 项目说明
├── FRAMEWORK.md                # 本文档
│
├── include/                    # 头文件
│   ├── Log.hpp                 # [已有] 日志系统
│   ├── Server.hpp              # [已有] 服务器主体
│   ├── Epoll.hpp               # [待实现] epoll 封装
│   ├── Channel.hpp             # [待实现] 事件通道（fd + 回调）
│   ├── HttpConn.hpp            # [待实现] HTTP 连接管理
│   ├── HttpRequest.hpp         # [待实现] HTTP 请求解析
│   ├── HttpResponse.hpp        # [待实现] HTTP 响应生成
│   ├── HttpRouter.hpp          # [待实现] 路由分发
│   ├── ThreadPool.hpp          # [待实现] 线程池
│   ├── Timer.hpp               # [待实现] 定时器
│   └── ConnectionPool.hpp      # [待实现] 数据库连接池
│
├── src/                        # 源文件（与 include 一一对应）
│   ├── main.cpp                # [已有] 入口
│   ├── Server.cpp              # [已有] 服务器实现
│   ├── Log.cpp                 # [已有] 日志实现
│   ├── Epoll.cpp
│   ├── Channel.cpp
│   ├── HttpConn.cpp
│   ├── HttpRequest.cpp
│   ├── HttpResponse.cpp
│   ├── HttpRouter.cpp
│   ├── ThreadPool.cpp
│   ├── Timer.cpp
│   └── ConnectionPool.cpp
│
├── build/                      # 构建产物（gitignore）
├── www/                        # 静态文件根目录
│   ├── index.html
│   ├── 404.html
│   └── static/
│       ├── css/
│       └── js/
│
├── tests/                      # 单元测试
│   ├── test_http_parser.cpp
│   ├── test_thread_pool.cpp
│   └── test_timer.cpp
│
├── bench/                      # 压力测试脚本
│   ├── wrk_script.lua
│   └── results/
│
└── .claude/                    # Claude Code 配置
    └── skills/                 # [已有] 项目技能
```

---

## 五、模块详细设计

### 5.1 Epoll 模块

**职责**：封装 Linux epoll，提供 I/O 多路复用接口。

```cpp
// include/Epoll.hpp
#pragma once
#include <sys/epoll.h>
#include <vector>

class Channel;  // 前向声明

class Epoll {
public:
    static constexpr int MAX_EVENTS = 4096;

    Epoll();
    ~Epoll();

    void AddChannel(Channel* channel);
    void ModChannel(Channel* channel);
    void DelChannel(Channel* channel);

    std::vector<Channel*> Poll(int timeout_ms = -1);

private:
    int epoll_fd_;
    struct epoll_event events_[MAX_EVENTS];
};
```

**设计要点**：
- 使用 `EPOLLET`（边缘触发）
- `Channel*` 作为 `epoll_event.data.ptr` 传递
- 不持有 Channel 的所有权

---

### 5.2 Channel 模块

**职责**：将 fd 与事件回调绑定，是事件分发的基本单元。

```cpp
// include/Channel.hpp
#pragma once
#include <functional>
#include <cstdint>

class Channel {
public:
    using EventCallback = std::function<void()>;

    Channel(int fd);

    void SetReadCallback(EventCallback cb);
    void SetWriteCallback(EventCallback cb);
    void SetCloseCallback(EventCallback cb);
    void SetErrorCallback(EventCallback cb);

    void EnableReading();
    void EnableWriting();
    void DisableReading();
    void DisableWriting();

    int GetFd() const;
    uint32_t GetEvents() const;
    void SetRevents(uint32_t events);

    void HandleEvent();  // 根据 revents_ 分发回调

private:
    int fd_;
    uint32_t events_;    // 关注的事件
    uint32_t revents_;   // epoll 返回的就绪事件

    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};
```

**事件常量**：
```
EPOLLIN  → read_callback_
EPOLLOUT → write_callback_
EPOLLERR → error_callback_
EPOLLHUP → close_callback_
```

---

### 5.3 HttpConn 模块

**职责**：管理单个 HTTP 连接的生命周期（读→解析→处理→写→关闭/复用）。

```cpp
// include/HttpConn.hpp
#pragma once
#include "Channel.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>

class HttpConn {
public:
    HttpConn(int fd);
    ~HttpConn();

    void Init();             // 重置状态（Keep-Alive 复用）
    Channel& GetChannel();

    void OnRead();           // EPOLLIN → 读数据 + 解析
    void OnWrite();          // EPOLLOUT → 发送响应
    void OnClose();          // 关闭连接
    void Process();          // 解析完成后，在线程池中执行

private:
    int fd_;
    Channel channel_;
    HttpRequest request_;
    HttpResponse response_;

    std::string read_buffer_;    // 读缓冲区
    std::string write_buffer_;   // 写缓冲区

    void HandleRequest();        // 路由 + 生成响应
};
```

**关键流程**：
```
OnRead():
  read(fd) → read_buffer_ → request_.Parse(read_buffer_)
  if COMPLETE → Process()

Process():
  Router::Match(request_) → Handler → response_
  write_buffer_ = response_.ToString()
  channel_.EnableWriting()

OnWrite():
  send(fd, write_buffer_) → 全部发送?
    → Keep-Alive → Init(), 等待下一个请求
    → Close → OnClose()
```

---

### 5.4 HttpRequest 模块

**职责**：HTTP/1.1 请求解析（增量状态机）。

```cpp
// include/HttpRequest.hpp
#pragma once
#include <string>
#include <unordered_map>

class HttpRequest {
public:
    enum class Method  { GET, POST, PUT, DELETE, HEAD, OPTIONS, UNKNOWN };
    enum class State   { REQUEST_LINE, HEADERS, BODY, COMPLETE, ERROR };

    void Parse(const char* data, size_t len);
    void Reset();

    Method GetMethod() const;
    std::string_view GetPath() const;
    std::string_view GetQuery() const;         // ?key=value
    std::string_view GetVersion() const;
    std::string_view GetHeader(const std::string& key) const;
    std::string_view GetBody() const;
    State GetState() const;

    static const char* MethodToString(Method m);

private:
    State state_ = State::REQUEST_LINE;
    Method method_ = Method::UNKNOWN;
    std::string path_;
    std::string query_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    size_t content_length_ = 0;

    bool ParseRequestLine(const std::string& line);
    bool ParseHeader(const std::string& line);
};
```

**解析状态机**：
```
               ┌──────────────────────────────────────────┐
               │                                          │
REQUEST_LINE ──► HEADERS ──► (has body?) ──► BODY ──► COMPLETE
     │             │              │
     └─────────────┴──────────────┴──────► ERROR
```

**请求行格式**：`GET /path?key=val HTTP/1.1\r\n`

---

### 5.5 HttpResponse 模块

**职责**：构建 HTTP/1.1 响应报文。

```cpp
// include/HttpResponse.hpp
#pragma once
#include <string>
#include <unordered_map>

class HttpResponse {
public:
    void SetStatusCode(int code);
    void SetHeader(const std::string& key, const std::string& value);
    void SetBody(const std::string& body);
    void SetBody(const char* data, size_t len);
    void SetContentType(const std::string& type);
    void SetKeepAlive(bool on);

    std::string ToString() const;
    void Reset();

    // 预设响应
    static HttpResponse Ok(const std::string& body = "");
    static HttpResponse NotFound();
    static HttpResponse InternalError();
    static HttpResponse BadRequest();

private:
    int status_code_ = 200;
    std::string status_text_ = "OK";
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

    static const char* StatusText(int code);
};
```

**响应格式**：
```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 13\r\n
Connection: keep-alive\r\n
\r\n
Hello WebServer
```

---

### 5.6 HttpRouter 模块

**职责**：URL 路由匹配，将请求分发到对应的处理函数。

```cpp
// include/HttpRouter.hpp
#pragma once
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <functional>
#include <unordered_map>
#include <string>

class HttpRouter {
public:
    using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;

    void Get(const std::string& path, Handler handler);
    void Post(const std::string& path, Handler handler);

    void Route(const HttpRequest& request, HttpResponse& response);

private:
    // method → { path → handler }
    std::unordered_map<std::string,
        std::unordered_map<std::string, Handler>> routes_;

    Handler not_found_handler_;
    void HandleStaticFile(const HttpRequest& req, HttpResponse& resp);
    std::string GetContentType(const std::string& ext);
};
```

**路由表结构**：
```
GET  /        → index_handler
GET  /hello   → hello_handler
POST /api/... → api_handler
GET  /*       → static_file_handler (fallback)
```

---

### 5.7 ThreadPool 模块

**职责**：固定大小线程池，异步执行任务。

```cpp
// include/ThreadPool.hpp
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stop_token>   // C++20

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // 提交任务，返回 std::future
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    // 禁止拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    std::vector<std::jthread> workers_;              // C++20 自动 join
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable_any cv_;                  // 支持 stop_token
    bool stop_ = false;
};
```

**Submit 实现**：
```cpp
template<typename F, typename... Args>
auto ThreadPool::Submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
{
    using ReturnType = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<ReturnType> result = task->get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return result;
}
```

---

### 5.8 Timer 模块

**职责**：管理连接超时，关闭空闲连接。

```cpp
// include/Timer.hpp
#pragma once
#include <chrono>
#include <queue>
#include <vector>
#include <unordered_map>
#include <functional>

struct TimerNode {
    int fd;
    std::chrono::steady_clock::time_point expire;
    std::function<void()> callback;

    bool operator>(const TimerNode& rhs) const {
        return expire > rhs.expire;
    }
};

class Timer {
public:
    using Clock = std::chrono::steady_clock;

    void Add(int fd, int timeout_ms, std::function<void()> cb);
    void Adjust(int fd, int timeout_ms);    // 刷新超时
    void Remove(int fd);
    void Tick();                              // 处理所有到期定时器
    int GetNextTimeout() const;               // 供 epoll_wait 使用

private:
    std::priority_queue<TimerNode, std::vector<TimerNode>, std::greater<>> heap_;
    std::unordered_map<int, bool> deleted_;   // 懒删除标记
};
```

**与事件循环集成**：
```cpp
while (running_) {
    int timeout = timer_.GetNextTimeout();    // -1 表示无定时器
    auto channels = epoll_.Poll(timeout);
    for (auto* ch : channels) {
        ch->HandleEvent();
    }
    timer_.Tick();                            // 关闭超时连接
}
```

---

### 5.9 Log 模块（升级）

**职责**：线程安全、异步、带日志级别的日志系统。

**升级目标**：

```
现有：
  Log::Print(level, msg)  → stdout

目标：
  Logger::Instance().Log(level, file, line, func, msg)
  ├── 线程安全（mutex）
  ├── 日志级别：TRACE < DEBUG < INFO < WARN < ERROR < FATAL
  ├── 输出目标：stdout + 文件
  ├── 日志轮转：按文件大小
  └── 异步刷盘：缓冲 + 定时 flush
```

**日志格式**：
```
2024-01-15 14:30:45.123 [INFO ] Server.cpp:42 Start() — Server listening on port 8080
```

---

### 5.10 ConnectionPool 模块

**职责**：MySQL 连接池，复用数据库连接。

```cpp
// include/ConnectionPool.hpp
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

class MySqlConnection {
public:
    MySqlConnection(const std::string& host, int port,
                    const std::string& user, const std::string& pass,
                    const std::string& db);
    ~MySqlConnection();
    bool Connect();
    void Disconnect();
    bool Ping();
    bool Execute(const std::string& sql);
private:
    void* conn_;   // MYSQL*
};

class ConnectionPool {
public:
    static ConnectionPool& Instance();

    void Init(const std::string& host, int port,
              const std::string& user, const std::string& pass,
              const std::string& db, size_t pool_size);

    std::shared_ptr<MySqlConnection> Acquire();
    void Release(std::shared_ptr<MySqlConnection> conn);

private:
    ConnectionPool() = default;
    std::queue<std::shared_ptr<MySqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t max_size_ = 8;
};
```

**RAII 用法**：
```cpp
auto conn = ConnectionPool::Pool::Instance().Acquire();
conn->Execute("SELECT * FROM users");
// 作用域结束自动 Release
```

---

## 六、实施路线图

### Phase 1：事件驱动基础

| 步骤 | 模块 | 说明 | 依赖 |
|------|------|------|------|
| 1.1 | Log 升级 | 补全日志级别，线程安全 | 无 |
| 1.2 | Epoll | epoll 封装 | Log |
| 1.3 | Channel | fd + 事件回调 | Epoll |
| 1.4 | Server 改造 | accept 循环 → epoll 事件循环 | Epoll, Channel |

**验收标准**：多客户端可同时连接，不阻塞。

### Phase 2：HTTP 协议

| 步骤 | 模块 | 说明 | 依赖 |
|------|------|------|------|
| 2.1 | HttpRequest | 请求解析状态机 | 无 |
| 2.2 | HttpResponse | 响应构建 | 无 |
| 2.3 | HttpConn | 连接生命周期管理 | Channel, Http*, Timer |
| 2.4 | HttpRouter | 路由分发 + 静态文件 | HttpRequest, HttpResponse |

**验收标准**：浏览器可访问 `http://host:port/`，返回静态页面。

### Phase 3：并发与定时

| 步骤 | 模块 | 说明 | 依赖 |
|------|------|------|------|
| 3.1 | ThreadPool | 线程池实现 | 无 |
| 3.2 | Timer | 连接超时管理 | 无 |
| 3.3 | 集成 | 事件循环 + 线程池 + 定时器 | 全部 |

**验收标准**：wrk 压测 > 10,000 req/s，空闲连接自动关闭。

### Phase 4：高级特性

| 步骤 | 模块 | 说明 | 依赖 |
|------|------|------|------|
| 4.1 | ConnectionPool | MySQL 连接池 | 无 |
| 4.2 | Log 异步 | 异步刷盘 + 日志轮转 | Log |
| 4.3 | 信号处理 | SIGINT/SIGTERM 优雅退出 | Server |

**验收标准**：数据库查询正常，日志不丢失，Ctrl+C 优雅退出。

### Phase 5：测试与优化

| 步骤 | 内容 | 说明 |
|------|------|------|
| 5.1 | 单元测试 | HttpParser, ThreadPool, Timer |
| 5.2 | 压力测试 | wrk 万级并发 |
| 5.3 | 性能分析 | perf / flamegraph |
| 5.4 | 优化 | 锁优化、内存池、sendfile |

**验收标准**：wrk > 50,000 req/s，P99 < 50ms，零内存泄漏。

---

## 七、技术规范

### 7.1 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 类名 | PascalCase | `HttpConn`, `ThreadPool` |
| 方法 | PascalCase | `OnRead()`, `GetChannel()` |
| 成员变量 | snake_case + `_` 后缀 | `fd_`, `read_buffer_` |
| 局部变量 | snake_case | `connfd`, `listen_fd` |
| 常量 | kPascalCase | `kMaxEvents`, `kMaxBufferSize` |
| 宏 | UPPER_SNAKE | `LOG_DEBUG(msg)` |
| 文件名 | PascalCase.hpp/cpp | `HttpConn.hpp`, `HttpConn.cpp` |

### 7.2 代码风格

- 缩进：4 空格
- 花括号：换行（Allman 风格）
- 每个类独立一对 `.hpp` / `.cpp`
- 头文件使用 `#pragma once`
- 优先使用 `std::string_view`（只读参数）、`std::span`（只读缓冲区）
- 优先使用智能指针，禁止裸 `new/delete`
- 使用 `enum class` 而非 `enum`

### 7.3 编译选项

```cmake
# CMakeLists.txt 最终形态
cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译警告
add_compile_options(-Wall -Wextra -Wpedantic)

# Debug/Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
else()
    add_compile_options(-O2)
endif()

# Sanitizers（可选）
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

include_directories(include)
aux_source_directory(src SRC)

add_executable(server ${SRC})

# 链接库
target_link_libraries(server pthread)       # 线程
# target_link_libraries(server mysqlclient) # 数据库（后续启用）
```

### 7.4 错误处理策略

```
系统调用失败 → 记录日志 + 关闭资源 + 返回
网络错误     → 记录日志 + 关闭连接 + 移除定时器
解析错误     → 返回 400 Bad Request
路由未匹配   → 返回 404 Not Found
内部异常     → 捕获 + 返回 500 + 记录日志
```

### 7.5 线程模型

```
Main Thread:
  - 负责所有 I/O（accept, read, write）
  - epoll_wait 驱动事件循环
  - 解析 HTTP 请求（CPU 轻量）

Worker Threads:
  - 执行业务逻辑（路由 handler）
  - 访问数据库
  - 生成响应内容

Logger Thread:
  - 异步写日志文件
  - 定时 flush
```

**关键约束**：I/O 操作只在主线程，业务逻辑只在工作线程，避免跨线程 fd 操作。

---

## 八、性能目标

| 指标 | 目标值 | 测试条件 |
|------|--------|----------|
| 并发连接数 | > 10,000 | wrk -c10000 |
| QPS（Hello World） | > 50,000 | wrk -t4 -c1000 -d30s |
| P99 延迟 | < 50ms | wrk 压测 |
| 内存占用 | < 100MB | 10k 空闲连接 |
| 启动时间 | < 100ms | 冷启动 |
