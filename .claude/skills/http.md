---
name: http
description: Implement or modify HTTP/1.1 parsing and response generation. Use when working on request parsing, routing, headers, or response formatting.
---

# HTTP Skill

Guide for implementing HTTP/1.1 protocol handling.

## HTTP/1.1 Request Format

```
GET /path HTTP/1.1\r\n
Host: localhost:8080\r\n
Connection: keep-alive\r\n
Content-Length: 0\r\n
\r\n
[body]
```

## Components

### 1. HttpRequest (`HttpRequest.hpp`)

```cpp
#include <string>
#include <unordered_map>

class HttpRequest {
public:
    enum class Method { GET, POST, PUT, DELETE, HEAD, OPTIONS, UNKNOWN };
    enum class State { REQUEST_LINE, HEADERS, BODY, COMPLETE, ERROR };

    void Parse(const char* data, size_t len);

    Method GetMethod() const;
    std::string_view GetPath() const;
    std::string_view GetVersion() const;
    std::string_view GetHeader(const std::string& key) const;
    std::string_view GetBody() const;
    std::string_view GetQuery() const;    // after '?'
    State GetState() const;

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

### 2. HttpResponse (`HttpResponse.hpp`)

```cpp
class HttpResponse {
public:
    void SetStatusCode(int code);
    void SetHeader(const std::string& key, const std::string& value);
    void SetBody(const std::string& body);
    void SetBody(const char* data, size_t len);

    std::string ToString() const;

    // Static helpers
    static HttpResponse Ok(const std::string& body = "");
    static HttpResponse NotFound();
    static HttpResponse InternalError();

private:
    int status_code_ = 200;
    std::string status_text_ = "OK";
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};
```

### 3. HttpParser State Machine

```
REQUEST_LINE ──► HEADERS ──► BODY ──► COMPLETE
     │              │          │
     └──────────────┴──────────┴──► ERROR
```

Parse incrementally (buffer partial data across multiple `recv` calls).

## Status Codes

| Code | Text             | When to use                    |
|------|------------------|--------------------------------|
| 200  | OK               | Successful request             |
| 301  | Moved Permanently| Redirect                       |
| 400  | Bad Request      | Malformed request              |
| 404  | Not Found        | Path not matched               |
| 405  | Method Not Allowed| Wrong HTTP method for path    |
| 500  | Internal Server Error | Handler threw exception   |

## Response Format

```
HTTP/1.1 200 OK\r\n
Content-Length: 13\r\n
Content-Type: text/plain\r\n
Connection: keep-alive\r\n
\r\n
Hello WebServer
```

## Keep-Alive

HTTP/1.1 defaults to keep-alive. After sending a response:
- If `Connection: close` header present, close the connection
- Otherwise, keep the fd open and reset the parser for the next request

## Static File Serving

```cpp
// Map URL path to filesystem
// /index.html  →  ./www/index.html
// /style.css   →  ./www/style.css

// Content-Type mapping
// .html → text/html
// .css  → text/css
// .js   → application/javascript
// .json → application/json
// .png  → image/png
// .jpg  → image/jpeg
```

## Important

- Handle partial reads (buffer incomplete requests)
- Validate Content-Length against actual body size
- Limit max header size (e.g., 8KB) to prevent abuse
- Use `std::string_view` where possible to avoid copies
