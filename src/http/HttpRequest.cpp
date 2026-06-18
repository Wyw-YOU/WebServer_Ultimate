#include "http/HttpRequest.hpp"
#include "Log.hpp"

#include <sstream>

    // 清空上次的残留信息
void HttpRequest::Reset()
{
    method_.clear();
    path_.clear();
    version_.clear();

    body_.clear();
    headers_.clear();

    methodType_ = HttpMethod::UNKNOWN;
}

const std::string& HttpRequest::Method() const
{
    return method_;
}
const std::string& HttpRequest::Path() const
{
    return path_;
}
const std::string& HttpRequest::Version() const
{
    return version_;
}

std::string HttpRequest::GetHeader(const std::string& key) const
{
    auto it = headers_.find(key);

    if(it != headers_.end())
    {
        return it->second;
    }

    return "";
}

// 判断是否是长连接
bool HttpRequest::IsKeepAlive() const
{
    auto it = headers_.find("Connection");

    if(version_ == "HTTP/1.1")
    {
        if(it == headers_.end())
            return true;

        return it->second != "close";
    }

    if(version_ == "HTTP/1.0")
    {
        if(it == headers_.end())
            return false;

        return it->second == "keep-alive";
    }

    return false;
}

// 解析请求行
bool HttpRequest::ParseRequestLine(const std::string& line)
{
    std::stringstream requestLine(line);

    requestLine >> method_ >> path_ >> version_;

    if(method_ == "GET")
    {
        methodType_ = HttpMethod::GET;
    }
    else if(method_ == "POST")
    {
        methodType_ = HttpMethod::POST;
    }
    else if(method_ == "PUT")
    {
        methodType_ = HttpMethod::PUT;
    }
    else if(method_ == "DELETE")
    {
        methodType_ = HttpMethod::DELETE_;
    }
    else
    {
        methodType_ = HttpMethod::UNKNOWN;
    }

    if(method_.empty() || path_.empty() || version_.empty())
    {
        return false;
    }

    return true;
}

// 添加头
bool HttpRequest::AddHeader(const std::string& line)
{
    auto pos = line.find(':');

    if(pos == std::string::npos)
    {
        return false;
    }

    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    while(!value.empty() && value.front() == ' ')
    {
        value.erase(value.begin());
    }

    headers_[key] = value;

    return true;
}

// body追加
void HttpRequest::AppendBody(const std::string& body)
{
    body_ += body;
}

const std::string& HttpRequest::Body() const
{
    return body_;
}

void HttpRequest::SetBody(const std::string& body)
{
    body_ = body;
}
void HttpRequest::SetHeader(const std::string& key, const std::string& value)
{
    headers_[key] = value;
}

bool HttpRequest::ParseStartLine(const std::string& line)
{
    return ParseRequestLine(line);
}

size_t HttpRequest::ContentLength() const
{
    auto it = headers_.find("Content-Length");

    if(it == headers_.end())
    {
        return 0;
    }

    return static_cast<size_t>(
        std::stoul(it->second)
    );
}

HttpMethod HttpRequest::MethodType() const
{
    return methodType_;
}