# 功能需求跟踪

## 已完成

| 功能 | 状态 | 说明 | 相关文件 |
|------|------|------|---------|
| 基础 TCP 服务器 | ✅ | bind/listen/accept | `Acceptor.hpp/cpp` |
| epoll 多路复用 (ET) | ✅ | 边缘触发 + 非阻塞 I/O | `Epoll.hpp/cpp` |
| Channel 事件分发 | ✅ | fd ↔ 回调绑定 | `Channel.hpp/cpp` |
| EventLoop Reactor | ✅ | 单线程事件循环 | `EventLoop.hpp/cpp` |
| 多 Reactor IO 线程池 | ✅ | Main Thread 接受连接，IO Thread Pool 处理读写 | `EventLoopThreadPool.hpp/cpp` |
| HTTP/1.1 增量解析 | ✅ | 状态机解析器，支持半包/粘包 | `HttpContext.hpp/cpp` |
| Keep-Alive 长连接 | ✅ | HTTP/1.1 默认 keep-alive | `Connection.cpp` |
| URL 路由 | ✅ | GET/POST 精确匹配 | `Router.hpp/cpp` |
| 静态文件服务 | ✅ | resources 目录，10MB 大小保护 | `Connection.cpp`, `FileUtil.hpp/cpp` |
| 最小堆定时器 | ✅ | 60s 连接超时，O(1) 按 fd 查找 | `Timer.hpp/cpp` |
| sendfile 零拷贝 | ✅ | 静态文件绕过用户空间拷贝 | `Connection.cpp`, `HttpResponse.hpp/cpp` |
| 信号处理 + 优雅退出 | ✅ | SIGINT / SIGTERM 安全退出 | `Server.hpp/cpp` |
| 异步日志 | ✅ | 双缓冲 + 独立写线程，IO 线程零阻塞 | `AsyncLogger.hpp/cpp`, `Log.hpp/cpp` |

## 待实现

| 功能 | 优先级 | 预期收益 | 说明 |
|------|--------|----------|------|
| HTTP gzip 压缩 | P1 | 带宽减 60-80% | zlib 压缩响应体 |
| HTTP 管线化 | P1 | 同连接多请求吞吐提升 | `HasPendingRequest` 逻辑已预留 |
| 内存池 | P2 | 减少 malloc 开销 | Buffer / Connection 对象复用 |
| 日志写文件 | P2 | 生产环境持久化 | 文件输出 + 日志轮转 |
| 数据库连接池 | P3 | MySQL 连接复用 | 需要 mysqlclient 依赖 |
| HTTPS | P3 | 加密通信 | OpenSSL 集成 |

## 性能里程碑

| 版本 | QPS | 关键变更 |
|------|-----|---------|
| 单线程 epoll | ~3000 | 基础 ET 模式 + Timer |
| Connection 层下沉 | ~22000 | 读写逻辑迁移到 Connection |
| 多 Reactor (有 bug) | ~700 | HttpTask/HttpResult 跨线程数据竞争 |
| 移除 Worker Pool | ~21000 | 业务处理回归 IO 线程，消除竞争 |
| sendfile + 异步日志 | 待测 | 零拷贝 + 日志不阻塞 IO 线程 |
