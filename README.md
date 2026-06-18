# WebServer_Ultimate

基于 C++14 / Linux epoll 的高性能 HTTP 服务器，采用多 Reactor 模型，实测 QPS 21000+。

## 核心特性

- 多 Reactor 架构：Main Thread 接受连接，IO Thread Pool 处理读写
- epoll 边缘触发 (ET) + 非阻塞 I/O
- HTTP/1.1 协议解析（增量状态机，支持半包/粘包）
- Keep-Alive 长连接复用
- 静态文件服务（10MB 大小保护）
- URL 路由（GET/POST 精确匹配）
- 最小堆定时器（连接超时管理）
- 分级日志系统（可编译期关闭）

## 技术栈

| 项目 | 说明 |
|------|------|
| 语言 | C++14 |
| 构建 | CMake 3.10+ |
| 平台 | Linux（epoll + eventfd + POSIX sockets） |
| 依赖 | 无外部库，纯系统调用 |

## 架构

```
┌──────────────────── Main Thread ────────────────────┐
│  EventLoop                                          │
│  ┌─────────┐   Accept    ┌───────────────────────┐  │
│  │Acceptor  │───────────▶│ Round-robin 选 IO Loop │  │
│  └─────────┘             └───────────┬───────────┘  │
└──────────────────────────────────────┼──────────────┘
                                       │
┌────────────── IO Thread Pool x20 ────┼──────────────┐
│  EventLoopThread[i]                  │              │
│  ┌──────────┐   EPOLLIN   ┌──────────────────────┐ │
│  │Connection│─────────────▶│ HandleRead            │ │
│  │          │              │ → Read → Process      │ │
│  │          │              │ → QueueInLoop(TrySend)│ │
│  │          │   EPOLLOUT   │                       │ │
│  │          │◀─────────────│ HandleWrite            │ │
│  └──────────┘              └──────────────────────┘ │
│  Timer (min-heap, 60s timeout)                      │
└─────────────────────────────────────────────────────┘
```

### 请求处理流程

```
1. 客户端连接
   → epoll_wait 检测 listenFd EPOLLIN (ET)
   → accept 循环到 EAGAIN
   → round-robin 选择 IO Loop
   → 创建 Connection，注册 epoll + 60s 定时器

2. 数据到达
   → epoll_wait 检测 fd EPOLLIN (ET)
   → Connection::HandleRead → Read (recv 循环到 EAGAIN)
   → ProcessInWorker (IO 线程内同步执行)
     → HttpContext 增量解析请求
     → Router 路由 / 静态文件服务
     → HttpResponse 序列化 → writeBuffer
   → QueueInLoop(TrySend)

3. 响应发送
   → epoll_wait 检测 fd EPOLLOUT (ET)
   → Connection::HandleWrite → Write (send 循环到 EAGAIN)
   → Keep-Alive → ResetForNextRequest，重新监听 EPOLLIN
   → 非 Keep-Alive → HandleClose

4. 超时 (60s)
   → Timer::Tick → RemoveConnection → close(fd)
```

## 项目结构

```
WebServer_Ultimate/
├── CMakeLists.txt
├── include/
│   ├── Server.hpp                    # 服务器主控（Acceptor + IO 池）
│   ├── Log.hpp                       # 分级日志（NORMAL / DEBUG / ERROR）
│   ├── buffer/
│   │   └── Buffer.hpp                # 读写缓冲区（string + 双游标）
│   ├── net/
│   │   ├── Acceptor.hpp              # bind / listen / accept
│   │   ├── Channel.hpp               # fd ↔ epoll 事件回调绑定
│   │   ├── Connection.hpp            # 连接生命周期管理（核心）
│   │   ├── Epoll.hpp                 # epoll_create1 / ctl / wait
│   │   ├── EventLoop.hpp             # Reactor 核心循环
│   │   ├── EventLoopThread.hpp       # 一个线程一个 EventLoop
│   │   ├── EventLoopThreadPool.hpp   # IO 线程池
│   │   ├── InetAddress.hpp           # sockaddr_in 封装
│   │   └── Socket.hpp                # RAII socket fd
│   ├── http/
│   │   ├── HttpContext.hpp            # 增量状态机 HTTP 解析器
│   │   ├── HttpRequest.hpp           # 请求数据模型
│   │   ├── HttpResponse.hpp          # 响应构建 + 序列化
│   │   ├── MimeType.hpp              # 扩展名 → MIME 映射
│   │   └── Router.hpp                # 路径路由（GET / POST）
│   ├── timer/
│   │   └── Timer.hpp                 # 最小堆定时器
│   └── util/
│       ├── Error.hpp                 # strerror 封装
│       └── FileUtil.hpp              # 文件读取 / stat
├── src/                              # 对应实现文件
├── resources/                        # 静态资源
│   ├── index.html / 404.html / hello.html / wrk_test.html
│   ├── css/main.css
│   ├── js/app.js
│   └── images/
└── build/
```

## 模块说明

### EventLoop — Reactor 核心

每个 EventLoop 持有独立的 Epoller、Timer、eventfd 唤醒通道和 Connection 映射表。

- `Loop()` — 主循环：epoll_wait → 分发事件 → 执行 pending functors → timer tick
- `QueueInLoop(cb)` — 线程安全的跨线程任务投递，通过 eventfd 唤醒
- `RunInLoop(cb)` — 同线程直接执行，异线程走 QueueInLoop

### Connection — 连接生命周期

状态机：`Connected → Processing → Writing → Closed`

管理完整的连接生命周期：读缓冲、HTTP 解析、路由、响应生成、写缓冲、Keep-Alive 复用。`shared_ptr` + `weak_ptr` 管理内存，防止循环引用。

### HttpContext — HTTP 解析器

增量状态机，处理半包和粘包：

```
REQUEST_LINE → HEADERS → BODY → FINISH
```

每一步都检查数据完整性，数据不足时返回 `Incomplete`，下次数据到达时继续解析。

### Timer — 最小堆定时器

基于 `std::vector` 的最小堆，`unordered_map` 实现 O(1) 按 fd 查找。

- `Add / Adjust / Delete` — 增删改 + 堆调整
- `GetNextTick()` — 处理已到期定时器，返回距下次超时的毫秒数（用作 epoll_wait timeout）

### Buffer — 读写缓冲区

基于 `std::string` 的双游标缓冲区（readPos / writePos）。

- `Append` — 写入数据，空间不足时自动扩容或整理
- `FindCRLF` — 扫描 `\r\n`（供 HTTP 解析器使用）
- `PeekReadable` — 零拷贝返回可读区域指针和长度（供 send 使用）

## 构建与运行

```bash
# 构建
mkdir build && cd build
cmake ..
make

# 运行
./server [端口号] [资源目录]

# 示例
./server 8080 ../resources
```

### 压力测试

```bash
# 安装 wrk
sudo apt install wrk

# 测试
wrk -t4 -c1000 -d30s http://localhost:8080/wrk_test.html
```

## 已注册路由

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/` | 返回 index.html |
| GET | `/hello` | 返回路由处理器响应 |
| POST | `/login` | 回显 POST body |
| GET | `/*` | 静态文件服务（resources 目录） |

## 线程安全设计

- **IO 线程**处理所有 I/O 和业务逻辑（Read → Parse → Route → Write），无跨线程竞争
- `EventLoop::QueueInLoop` 通过 mutex + eventfd 实现安全的跨线程回调投递
- `Connection` 使用 `shared_ptr` / `weak_ptr` 管理生命周期
- `HandleClose` 通过 `shared_from_this()` 捕获，防止 use-after-free
- `atomic<ConnState>` 保证状态可见性

## 性能

| 指标 | 数值 | 测试条件 |
|------|------|----------|
| QPS | 21000+ | wrk -t4 -c1000 -d30s，静态页面 |
| IO 线程数 | 20 | EventLoopThreadPool |
| 连接超时 | 60s | 最小堆定时器 |
| 文件大小限制 | 10MB | 超限返回 413 |

## 后续计划

- [ ] 日志异步化（独立日志线程 + 缓冲区）
- [ ] sendfile 零拷贝优化
- [ ] 连接池（数据库）
- [ ] HTTPS 支持（OpenSSL）
- [ ] 信号处理（优雅退出）
