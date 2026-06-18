---
name: audit
description: Deep architecture-aware code audit for the WebServer_Ultimate project. Checks Reactor pattern correctness, ET mode safety, HTTP compliance, resource lifecycle, and cross-module consistency. Use when asked to audit, do a deep review, or check architecture correctness.
---

# WebServer_Ultimate 架构审计

对项目全量源码进行架构级审计，关注 Reactor 模式、ET 模式、HTTP 协议、资源生命周期等维度。

## 审计流程

1. 读取所有 `include/**/*.hpp` 和 `src/**/*.cpp`（排除 build 目录）
2. 按以下 8 个维度逐项检查
3. 输出审计报告

## 审计维度

### 1. Reactor 模式正确性
- Channel 是否正确绑定 read/write/close 回调
- EventLoop 是否正确通过 `data.ptr` 分发事件到 Channel
- Channel::HandleEvent() 优先级：ERR/HUP 是否优先于 IN/OUT
- Server 是否将 I/O 和业务逻辑分离（当前可接受单线程，但检查是否预留线程池接口）
- Channel 生命周期是否与 Connection 同步创建/销毁

### 2. ET 模式安全性
- 所有 socket 是否设置了 `EPOLLET`
- 读操作是否循环 recv 直到 EAGAIN
- 写操作是否循环 send 直到发完或 EAGAIN
- 事件修改是否在每次处理后正确重置（ModChannel）
- 监听 socket 的 accept 是否循环直到 EAGAIN

### 3. HTTP/1.1 协议合规性
- 请求解析是否正确处理 `\r\n` 行尾
- 是否正确解析请求行（method path version）
- 是否正确解析头部（key: value，跳过空行）
- Content-Length 与实际 body 长度是否一致
- Keep-Alive 判断是否正确（HTTP/1.1 默认 keep-alive）
- 响应格式是否符合 RFC（状态行 + 头部 + 空行 + body）
- 是否返回必要的头部（Content-Length, Content-Type, Connection）

### 4. 资源生命周期
- fd 是否有唯一所有者（避免 double-close）
- Channel 从 epoll 注销是否在 fd 关闭之前
- Connection 和 Channel 的 map 是否同步清理
- Socket 析构是否正确关闭 fd
- 智能指针是否正确管理堆对象

### 5. Buffer 正确性
- readPos_/writePos_ 是否一致维护
- Peek() vs PeekReadable() 使用场景是否正确
- Empty() 是否检查 ReadableBytes 而非 buffer_.empty()
- MakeSpace 压缩逻辑是否正确（移动可读数据到头部）
- 扩容策略是否合理（2x 增长）

### 6. 错误处理
- 系统调用（socket/bind/listen/accept/recv/send/epoll_ctl）是否检查返回值
- errno 是否在系统调用后立即检查
- 错误路径是否正确释放资源
- 是否有未处理的异常路径

### 7. 跨模块一致性
- Epoller 的接口（AddChannel/ModChannel/DelChannel）是否与 Channel 的事件设置一致
- Server 的事件修改逻辑在 HandleReadEvent 和 HandleWriteEvent 中是否一致
- Connection 的 Read/Process/Write 返回值语义是否清晰
- 日志级别使用是否一致（NORMAL/DEBUG/ERROR）

### 8. 代码规范
- 命名规范：PascalCase 类/方法，snake_case_ 成员变量
- 头文件是否使用 `#pragma once`
- 是否有残留调试代码（std::cout, 注释掉的代码）
- 注释是否准确描述代码行为

## 输出格式

```markdown
# 架构审计报告

> 审计范围：全量源码
> 审计时间：YYYY-MM-DD

## 审计总览

| 维度 | 评分 | 问题数 |
|------|------|--------|
| Reactor 模式 | ★★★★☆ | N |
| ET 模式 | ★★★★☆ | N |
| HTTP 协议 | ★★★☆☆ | N |
| 资源生命周期 | ★★★★☆ | N |
| Buffer 正确性 | ★★★★☆ | N |
| 错误处理 | ★★★☆☆ | N |
| 跨模块一致性 | ★★★★☆ | N |
| 代码规范 | ★★★★☆ | N |

## 问题清单

### Critical（必须修复）
1. **[file:line]** — 问题描述

### Warning（建议修复）
1. **[file:line]** — 问题描述

### Suggestion（可选优化）
1. **[file:line]** — 建议描述

## 审计结论
一段话总结整体质量。
```
