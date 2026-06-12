#pragma once

#include "http/HttpRequest.hpp"
#include "buffer/Buffer.hpp"

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

class HttpContext
{
public:
    // 解析请求
    bool ParseRequest(Buffer& buffer);
    // 判断包是否完整
    bool IsComplete() const;
    // 获取请求
    HttpRequest& Request();
    // 清除请求的缓存
    void Reset();

    ParseState GetState() const;

private:
    HttpRequest request_;
    ParseState state_ = ParseState::REQUEST_LINE;
    bool complete_ = false;
    size_t contentLength_ = 0;
};