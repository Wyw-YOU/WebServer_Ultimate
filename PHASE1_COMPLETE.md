# 阶段一完成：XSS 修复 + 配置外部化

## ✅ 已完成的修改

### 1. XSS 漏洞修复

**创建文件：**
- `include/util/HtmlEscape.hpp` - HTML/JS 转义工具类

**修改文件：**
- `src/Server.cpp` - 应用转义函数

**修复内容：**
1. **BuildAlertPage 函数**（第 16-26 行）：
   - 对 `message` 参数应用 `EscapeJsString()` 转义
   - 对 `redirect` 参数应用 `EscapeHtml()` 转义
   - 防止通过用户名注入恶意 JavaScript

2. **用户名输出**（第 118 行）：
   - 对 `username` 应用 `EscapeHtml()` 转义后再拼接
   - 防止 XSS 注入

**安全防护：**
- ✅ 阻止 `<script>alert(1)</script>` 类型的 XSS 攻击
- ✅ 阻止通过单引号 `';alert(document.cookie);//` 注入 JS
- ✅ 转义所有 HTML 特殊字符（`& < > " '`）
- ✅ 转义 JS 字符串特殊字符（`\ ' " \n \r \t /`）
- ✅ 转义控制字符（ASCII < 0x20）

---

### 2. 配置外部化

**创建文件：**
- `include/util/Config.hpp` - 环境变量/配置读取工具
- `.env.example` - 配置示例文件

**修改文件：**
- `src/Server.cpp` - 使用 Config 读取数据库配置

**修复内容：**
1. **移除硬编码密码**：
   - 删除 `ConnectionPool::Init("127.0.0.1", 3306, "root", "Wyw962464.", "webserver", 8)`
   - 改为从环境变量读取所有配置

2. **支持的环境变量：**
   - `DB_HOST` - 数据库地址（默认：127.0.0.1）
   - `DB_PORT` - 数据库端口（默认：3306）
   - `DB_USER` - 数据库用户（默认：root）
   - `DB_PASSWORD` - 数据库密码（必填，无默认值）
   - `DB_NAME` - 数据库名称（默认：webserver）
   - `DB_POOL_SIZE` - 连接池大小（默认：8）

3. **错误处理：**
   - 如果 `DB_PASSWORD` 未设置，服务会报错并退出
   - 启动时日志显示数据库连接信息（不含密码）

---

## 📋 文件清单

| 操作 | 文件 | 说明 |
|------|------|------|
| ✨ 新增 | `include/util/HtmlEscape.hpp` | HTML/JS 转义工具（header-only） |
| ✨ 新增 | `include/util/Config.hpp` | 环境变量读取工具（header-only） |
| ✨ 新增 | `.env.example` | 配置示例文件 |
| ✏️ 修改 | `src/Server.cpp` | XSS 修复 + 配置外部化 |

---

## 🧪 验证方法

### XSS 验证
```bash
# 1. 注册恶意用户名
curl -X POST http://localhost:8080/login \
  -d "username=<script>alert(1)</script>&password=test"

# 2. 检查返回的 HTML
# 应该看到转义后的内容：&lt;script&gt;alert(1)&lt;/script&gt;
# 而不是原始的 <script>alert(1)</script>
```

### 配置验证
```bash
# 1. 不设置 DB_PASSWORD，启动服务（应该失败）
./server 8080
# 输出：Error: DB_PASSWORD environment variable is required

# 2. 设置环境变量后启动（应该成功）
export DB_PASSWORD=your_password
./server 8080
# 输出：Database config: root@127.0.0.1:3306/webserver (pool: 8)
```

---

## ⚠️ 注意事项

1. **Linux 环境**：项目使用 epoll 等 Linux 特有 API，只能在 Linux 上编译运行
2. **依赖库**：需要安装 mysqlclient 和 zlib
3. **环境变量持久化**：
   - 临时设置：`export DB_PASSWORD=xxx`
   - 永久设置：添加到 `~/.bashrc` 或 `~/.zshrc`
   - systemd 服务：使用 `EnvironmentFile` 配置
   - Docker：使用 `-e` 参数或 `docker-compose.yml`

---

## 📝 下一步

阶段一已完成。可以继续实施：
- **阶段二**：路径遍历加固 + 请求大小限制
- **阶段三**：密码哈希 + 错误信息治理

如需继续，请告知。
