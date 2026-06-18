# WebServer_Ultimate 代码结构与性能问题分析

## 一、项目概览

基于 C++14 / Linux epoll 的多 Reactor Web Server，支持 HTTP/1.0 & 1.1、Keep-Alive、静态文件服务、路由。

> 重构后简化：移除 HttpTask.hpp / HttpResult.hpp / workerPool，业务处理回归 IO 线程内直接执行。

```
WebServer_Ultimate/
├── include/
│   ├── Server.hpp              # 服务器主控（Acceptor + IO池）
│   ├── Log.hpp                 # 分级日志（可编译期关闭）
│   ├── buffer/Buffer.hpp       # 读写缓冲区（string + 双游标）
│   ├── net/
│   │   ├── Acceptor.hpp        # bind/listen/accept 封装
│   │   ├── Channel.hpp         # fd ↔ epoll 事件回调
│   │   ├── Connection.hpp      # 连接生命周期管理（核心）
│   │   ├── Epoll.hpp           # epoll_create1/ctl/wait 封装
│   │   ├── EventLoop.hpp       # Reactor 核心循环
│   │   ├── EventLoopThread.hpp # 一个线程一个 EventLoop
│   │   ├── EventLoopThreadPool.hpp  # IO 线程池
│   │   ├── InetAddress.hpp     # sockaddr_in 封装
│   │   └── Socket.hpp          # RAII socket fd
│   ├── http/
│   │   ├── HttpContext.hpp     # 增量状态机 HTTP 解析器
│   │   ├── HttpRequest.hpp     # 请求数据模型 ⚠️ 有遗留 Parse()
│   │   ├── HttpResponse.hpp    # 响应构建 + 序列化
│   │   ├── MimeType.hpp        # 扩展名→MIME映射
│   │   └── Router.hpp          # 路径路由（GET/POST精确匹配）
│   ├── thread/ThreadPool.hpp   # 固定大小线程池（默认128线程）
│   ├── timer/Timer.hpp         # 最小堆定时器
│   └── util/
│       ├── Error.hpp           # strerror 封装
│       └── FileUtil.hpp        # 文件读取/stat
├── src/                        # 对应实现文件
├── resources/                  # 静态资源（index/404/hello/css/js/images）
└── CMakeLists.txt              # C++14, -Wall -Wextra -pthread
```

---

## 二、线程模型

```
┌─────────────────────────────────────────────────────────────┐
│                      Main Thread                            │
│  EventLoop loop_                                            │
│  ┌─────────┐    Accept     ┌──────────────────────────┐     │
│  │Acceptor  │──────────────▶│ Round-robin 选择 IO Loop │     │
│  └─────────┘               └──────────┬───────────────┘     │
└───────────────────────────────────────┼─────────────────────┘
                                        │
┌──────────────── IO Reactor Threads × 20 ────────────────────┐
│  EventLoopThread[i]  →  EventLoop  →  Epoller + Timer       │
│  ┌──────────┐    EPOLLIN     ┌──────────────────────┐       │
│  │ Connection│───────────────▶│ HandleRead           │       │
│  │          │                │ → Read → Process      │       │
│  │          │                │ → QueueInLoop(TrySend)│       │
│  │          │    EPOLLOUT    │                      │       │
│  │          │◀───────────────│ HandleWrite           │       │
│  └──────────┘                └──────────────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

> 业务处理（HTTP 解析 + 路由）现在直接在 IO 线程内同步执行，无跨线程竞争。

---

## 三、请求完整生命周期

```
1. 客户端 TCP 连接
   └─▶ Main Loop epoll_wait 检测 listenFd EPOLLIN (ET模式)
       └─▶ HandleListenEvent() 循环 accept 直到 EAGAIN
           └─▶ 选择 ioLoop = ioPool_.GetNextLoop() (round-robin)
               └─▶ 创建 shared_ptr<Connection>(fd, ioLoop, ...)
                   └─▶ 设置 IO 回调（weak_ptr 防循环引用）
                   └─▶ ioLoop->AddConnection(conn)
                   └─▶ ioLoop->GetEpoller().AddChannel(conn->GetChannel())
                   └─▶ ioLoop->AddTimer(fd, 60000, ...)

2. 客户端发送数据
   └─▶ IO Loop epoll_wait 检测 fd EPOLLIN (ET模式)
       └─▶ Channel::HandleEvent() → Connection::HandleRead()
           └─▶ Connection::Read() 循环 recv 直到 EAGAIN → readBuffer_
           └─▶ 检查 state == Connected
               └─▶ SetState(Processing) + AdjustTimer(60s)
                   └─▶ ProcessInWorker() [在当前 IO 线程同步执行]
                       ├─▶ Process() → context_.ParseRequest(readBuffer_)
                       ├─▶ Router::Route() 或 FileUtil::ReadFile()
                       ├─▶ response_.ToString() → writeBuffer_
                       └─▶ QueueInLoop(TrySend)

3. IO Loop DoPendingFunctors → TrySend → SendResponse
   └─▶ Connection::HandleWrite() → Write() 循环 send 直到 EAGAIN
       └─▶ 写完 → DisableWriteEvent()
       └─▶ HandleWriteResult(WRITE_COMPLETE)
           └─▶ OnResponseFinished()
               ├─▶ Keep-Alive → ResetForNextRequest() → 重新监听 EPOLLIN
               └─▶ 非 Keep-Alive → HandleClose()

4. 定时器超时 (60s 无活动)
   └─▶ Timer::Tick() 触发回调
       └─▶ EventLoop::RemoveConnection(fd)
           └─▶ connections_.erase(fd) → Connection 析构 → close(fd)
```

---

## 四、已发现的 Bug 与性能问题

### ✅ 已修复：跨线程数据竞争 — PushResult 无锁保护（原 P0）

**状态**: 已通过移除 HttpTask/HttpResult/workerPool 解决。业务处理现在在 IO 线程内同步执行，消除跨线程竞争。

---

### ✅ 已修复：HandleClose 捕获裸指针 this（原 P0）

**修复**: 改为捕获 `shared_from_this()`。

```cpp
// 修复前
loop_->QueueInLoop([this]() { loop_->RemoveConnection(fd_); });

// 修复后
auto self = shared_from_this();
loop_->QueueInLoop([self]() { self->loop_->RemoveConnection(self->fd_); });
```

---

### ✅ 已修复：Timer 刷新指向错误 EventLoop（原 P1）

**修复**: Timer 刷新从 `Server::HandleReadEvent` 移至 `Connection::HandleRead`，直接使用 Connection 所属的 `loop_`（即正确的 IO Loop）。

```cpp
// 修复前 (Server.cpp - Main Loop 的 timer)
loop_.AdjustTimer(fd, 60000);

// 修复后 (Connection.cpp - IO Loop 的 timer)
loop_->AdjustTimer(fd_, 60000);
```

---

### 🟡 Bug #4（中等）：state_ 检查-修改竞态条件

**位置**: `Connection.cpp:297-301`（HandleRead 内）

```cpp
if(state_ == ConnState::Connected)   // ← 检查
{
    SetState(ConnState::Processing);  // ← 修改
```

**分析**: ET 模式下同一 fd 不会在两个 IO 线程同时触发，且 HandleRead 内同步调用 Process 不会产生并发，所以当前窗口极小。但形式上仍非原子操作。

---

### 🟡 Bug #5（中等）：HttpContext::ParseHeaderLine 重复调用 SetHeader

**位置**: `src/http/HttpContext.cpp:167,169`

```cpp
request_.SetHeader(key, value);  // 第一次
request_.SetHeader(key, value);  // 第二次（重复）
```

---

### 🟡 问题 #6：两套处理路径残留代码

`Process()` 和 `ProcessInWorker()` 中仍有旧架构的残留逻辑（如直接写 `writeBuffer_` 的分支）。可进一步清理统一。

---

### 🟢 问题 #7（低）：EventLoop 内嵌 ThreadPool 未使用

每个 `EventLoop` 对象内含一个 `ThreadPool threadPool_`（默认 128 线程），但完全未使用。

---

## 五、关于 HttpRequest.hpp

### HttpRequest.hpp

该文件仍然**需要保留**：
- `HttpContext` 的状态机解析器依赖 `HttpRequest` 存储解析结果（method, path, headers, body）
- `Router::Route()` 依赖 `HttpRequest` 做路由匹配
- `HttpRequest::IsKeepAlive()` 在响应阶段被调用
- `HttpRequest::Parse()` 是遗留的旧解析方法（标记了 TODO 待删除），但 `ParseStartLine`、`SetHeader`、`IsKeepAlive` 等方法仍然活跃使用

**建议**: 删除 `Parse()` 方法及其私有辅助函数（`ParseHeaders`、`ParseBody`、`ParseRequestLine`），保留其他接口。

### ~~HttpTask.hpp / HttpResult.hpp~~ （已移除）

这两个文件已在本次重构中删除。相关代码变更：
- 移除 `Connection::ProcessTask()`、`Connection::PushResult()`、`Connection::GetReadBufferCopy()`
- 移除 `Server::HandleReadEvent()` 和 `Server::workerPool_`
- `HandleRead()` 改为直接在 IO 线程内调用 `ProcessInWorker()`

---

## 六、剩余修复建议（按优先级排序）

### P2: 清理旧代码残留

- 删除 `HttpRequest::Parse()` 及其私有辅助方法
- 删除 `EventLoop::threadPool_`（未使用的成员）
- 清理 `Process()` 中重复的 writeBuffer 写入逻辑（路由分支和静态文件分支各写一次）

### P2: ParseHeaderLine 重复 SetHeader

删除 `HttpContext.cpp:167` 或 `169` 的重复调用。

---

## 七、QPS 暴跌根因分析

```
旧架构 (22000+ QPS):
  IO Threads × 20
    └─ HandleRead → Read → Process (同步) → writeBuffer → TrySend
  优点: 无跨线程竞争，无锁，上下文切换少

问题架构 (700 QPS):
  IO Threads × 20 (Read/Write)
  Worker Threads × 20 (ProcessTask → PushResult 直写 buffer)
  问题: PushResult 跨线程无锁写 buffer → 数据竞争 → 性能崩溃

已修复架构（当前）:
  IO Threads × 20
    └─ HandleRead → Read → ProcessInWorker(同步) → writeBuffer → QueueInLoop(TrySend)
  优点: 回归无锁，保留多 Reactor IO 架构
```

根因：`PushResult` 在 Worker 线程直接写 `writeBuffer_` 和调用 `epoll_ctl`，与 IO 线程并发操作同一资源，无任何同步保护。x86 上 cache line bouncing (MESI 协议) 导致严重性能退化。

现已通过移除 Worker 池和 HttpTask/HttpResult，将业务处理回退到 IO 线程内同步执行来解决。

---

## 八、架构演进路线

```
Phase 1-2: 单线程 epoll (LT → ET)
    ↓
Phase 3: Channel + EventLoop 抽象
    ↓
Phase 4: ThreadPool 业务并发
    ↓
Phase 5: Timer 最小堆 (QPS 3k+)
    ↓
Phase 6: Connection 层下沉 (QPS 22k+)
    ↓
Phase 7: HTTP 状态机解析 + Keep-Alive 修复
    ↓
Phase 8: 静态文件 + 多 Reactor (IO 线程池) ← 当前阶段
    ↓
下一步: 清理旧代码残留，性能测试验证 QPS 恢复
```
