#include "http/HttpRequest.hpp"
#include "Log.hpp"

#include <sstream>

// 解析HTTP请求
bool HttpRequest::Parse(const std::string& raw)
{
    // 解析HTTP请求行
    std::stringstream ss(raw);
    std::string line;

    if(!std::getline(ss, line))
    {
        LOG_ERROR("Failed to read request line!");
        return false;
    }

    if(!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }

    std::stringstream requestLine(line);

    requestLine >> method_ >> path_ >> version_;

    // 解析HTTP头部字段
    while(std::getline(ss, line))
    {
        if(line == "\r" || line.empty())
        {
            break;
        }

        if(line.back() == '\r')
        {
            line.pop_back();
        }

        auto pos = line.find(':');

        if(pos == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        while(!value.empty() && value.front() == ' ')
        {
            value.erase(value.begin());
        }

        headers_[key] = value;
    }

    // 解析HTTP请求体
    std::string bodyLine;

    while(std::getline(ss, bodyLine))
    {
        body_ += bodyLine;
    }

    LOG_DEBUG("Parsed HTTP request: Method: " + method_ + " Path: " + path_ + " Version: " + version_);
    LOG_DEBUG("Parsed Successfully!");

    return true;
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