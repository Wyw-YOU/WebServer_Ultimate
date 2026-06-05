#include "http/HttpResponse.hpp"

std::string HttpResponse::ToString() const
{
    std::stringstream ss;

    // 状态行
    ss << "HTTP/1.1 " << statusCode_ << " " << status_ << "\r\n";

    // 默认头部，如果没设置Content-Type，给text/plain
    if (headers_.find("Content-Type") == headers_.end())
        ss << "Content-Type: text/plain\r\n";

    // 头部
    for (auto it = headers_.begin(); it != headers_.end(); ++it)
    {
        const std::string& key = it->first;
        const std::string& value = it->second;
    
        ss << key << ": " << value << "\r\n";
    }

    // 空行
    ss << "\r\n";

    // body
    ss << body_;

    return ss.str();
}