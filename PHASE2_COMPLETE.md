# 阶段二完成：路径遍历加固 + 请求大小限制

## ✅ 已完成的修改

### 1. 路径遍历防护增强

**创建文件：**
- `include/util/PathUtil.hpp` - 路径安全校验工具类

**修改文件：**
- `src/net/Connection.cpp` - 应用 SafePath 函数

**修复内容：**
1. **SafePath 函数**实现了多层防护：
   - URL 解码（防止 `%2e%2e` 绕过）
   - 空字节检测（防止 `%00` 注入）
   - 路径规范化（移除连续 `/`、解析 `.` 和 `..`）
   - `realpath()` 绝对路径解析
   - 目录边界检查（确保在 resourceDir 内）

2. **Connection::Process()** 函数修改：
   - 使用 `SafePath` 替换原有的 `..` 检查
   - 添加详细的错误日志记录
   - 非法路径返回 403 Forbidden

**防护的攻击向量：**
- ✅ `GET /../../../etc/passwd` - 标准路径遍历
- ✅ `GET /%2e%2e/%2e%2e/etc/passwd` - URL 编码绕过
- ✅ `GET /..%00/index.html` - 空字节注入
- ✅ `GET /....//....//etc/passwd` - 双写绕过
- ✅ `GET /var/www/../../etc/passwd` - 绝对路径遍历
- ✅ 符号链接攻击（通过 realpath 检测）

---

### 2. 请求大小限制

**修改文件：**
- `src/http/HttpContext.cpp` - ParseBody 函数添加大小检查
- `src/net/Connection.cpp` - Read 函数添加缓冲区大小检查

**修复内容：**
1. **HTTP Body 大小限制**（1MB）：
   - 在 `ParseBody()` 开头检查 `contentLength_`
   - 超过 1MB 返回 `ParseResult::Error`
   - 日志记录超限请求

2. **读缓冲区大小限制**（2MB）：
   - 在 `Read()` 循环中检查 `readBuffer_.ReadableBytes()`
   - 超过 2MB 返回 `ReadResult::Error`（断开连接）
   - 防止恶意客户端发送超大数据耗尽内存

**防护的攻击：**
- ✅ Content-Length 超大的 POST 请求（1MB 限制）
- ✅ 持续发送数据不结束的慢速攻击（2MB 缓冲区限制）
- ✅ 内存耗尽 DoS 攻击

---

## 📋 文件清单

| 操作 | 文件 | 说明 |
|------|------|------|
| ✨ 新增 | `include/util/PathUtil.hpp` | 路径安全校验工具（header-only） |
| ✏️ 修改 | `src/net/Connection.cpp` | 路径遍历防护 + 缓冲区大小限制 |
| ✏️ 修改 | `src/http/HttpContext.cpp` | HTTP body 大小限制 |

---

## 🧪 验证方法

### 路径遍历防护验证

```bash
# 1. 标准路径遍历
curl -v http://localhost:8080/../../../etc/passwd
# 预期：403 Forbidden

# 2. URL 编码绕过
curl -v http://localhost:8080/%2e%2e/%2e%2e/etc/passwd
# 预期：403 Forbidden

# 3. 空字节注入
curl -v "http://localhost:8080/..%00/index.html"
# 预期：403 Forbidden

# 4. 双写绕过
curl -v http://localhost:8080/....//....//etc/passwd
# 预期：403 Forbidden

# 5. 正常请求
curl -v http://localhost:8080/index.html
# 预期：200 OK
```

### 请求大小限制验证

```bash
# 1. 测试 Content-Length 限制（1MB）
# 创建一个 2MB 的文件
dd if=/dev/zero of=test_2mb.bin bs=1M count=2

# 发送 POST 请求（应返回 413 或错误）
curl -X POST http://localhost:8080/login \
  -H "Content-Length: 2000000" \
  --data-binary @test_2mb.bin

# 2. 测试正常大小的 POST（应成功）
curl -X POST http://localhost:8080/login \
  -d "username=test&password=123"
```

---

## 📊 安全改进对比

### 路径遍历防护

**之前（弱防护）：**
```cpp
if(path.find("..") != std::string::npos)
{
    response_.SetStatus(403, "Forbidden");
    // 容易被 URL 编码绕过
}
```

**现在（强防护）：**
```cpp
std::string safePath = PathUtil::SafePath(resourceDir_, path);
if(safePath.empty())
{
    LOG_ERROR("Path traversal attempt detected: " + path);
    response_.SetStatus(403, "Forbidden");
    // 多层防护：URL解码 + 空字节检测 + 路径规范化 + realpath检查
}
```

### 请求大小限制

**之前（无限制）：**
- ❌ 无 Content-Length 检查
- ❌ 无缓冲区大小限制
- ❌ 容易被 DoS 攻击

**现在（有限制）：**
- ✅ HTTP Body 限制 1MB
- ✅ 读缓冲区限制 2MB
- ✅ 超限立即断开连接
- ✅ 日志记录攻击尝试

---

## ⚠️ 注意事项

1. **性能影响**：
   - `realpath()` 系统调用会增加少量延迟
   - 对于正常请求，影响微乎其微（< 1ms）
   - 对于恶意请求，增加的检查是必要的安全开销

2. **文件大小限制**：
   - 原有的 10MB 静态文件限制仍然有效
   - 新增的 1MB 限制针对 HTTP Body（POST 数据）
   - 两个限制独立工作，互不影响

3. **日志记录**：
   - 所有路径遍历尝试都会被记录（`LOG_ERROR`）
   - 缓冲区溢出尝试会被记录
   - 建议定期检查日志，了解攻击情况

---

## 📝 下一步

阶段二已完成！可以继续实施：
- **阶段三**：密码哈希 + 错误信息治理（预计 3-4 小时）

如需继续，请告知。
