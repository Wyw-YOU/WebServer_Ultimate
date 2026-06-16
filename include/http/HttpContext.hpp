#pragma once

#include "http/HttpRequest.hpp"
#include "buffer/Buffer.hpp"
#include "Log.hpp"

#include <algorithm>

// 本代码的意义：解决
// 半包
// 粘包
// 请求完整性判断
// 解析状态管理

enum class ParseState
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH
};

enum class ParseResult
{
    Complete,
    Incomplete,
    Error
};

class HttpContext
{
public:
    // 解析请求
    ParseResult ParseRequest(Buffer& buffer);
    // 判断包是否完整
    bool IsComplete() const;
    // 获取请求
    HttpRequest& Request();
    // 清除请求的缓存
    void Reset();
    // 获取状态
    ParseState GetState() const;


private:
ParseResult ParseRequestLine(Buffer& buffer);
ParseResult ParseHeaders(Buffer& buffer);
bool ParseHeaderLine(const std::string& line);
ParseResult ParseBody(Buffer& buffer);

private:
    HttpRequest request_;
    ParseState state_ = ParseState::REQUEST_LINE;
    bool complete_ = false;
    size_t contentLength_ = 0;
};