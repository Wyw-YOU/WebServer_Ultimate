#include "http/HttpContext.hpp"

    // 解析请求
bool HttpContext::ParseRequest(Buffer& buffer)
{
    auto readable = buffer.PeekReadable();
    std::string raw(readable.first, readable.second);
    auto pos = raw.find("\r\n\r\n");
    if(pos == std::string::npos)
    {
        complete_ = false;
        return false;
    }
    size_t requestLen = pos + 4;
    std::string requestData = buffer.Retrieve(requestLen);

    complete_ = request_.Parse(requestData);
    return complete_;
}

    // 判断包是否完整
bool HttpContext::IsComplete() const
{
    return complete_;
}

    // 获取请求
HttpRequest& HttpContext::Request()
{
    return request_;
}

    // 清除请求的缓存ss
void HttpContext::Reset()
{
    request_.Reset();
    state_ = ParseState::REQUEST_LINE;
    contentLength_ = 0;
    complete_ = false;
}

ParseState HttpContext::GetState() const
{
    return state_;
}