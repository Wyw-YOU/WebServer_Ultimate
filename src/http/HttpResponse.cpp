#include "http/HttpResponse.hpp"

std::string HttpResponse::ToString()
{
    BuildDefaultHeaders();
    std::stringstream ss;
    // 状态行
    ss << "HTTP/1.1 " << statusCode_ << " " << status_ << "\r\n";

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

void HttpResponse::SetStatus(int code, const std::string& status)
{
    statusCode_ = code;
    status_ = status;
}

void HttpResponse::SetBody(const std::string& body)
{
    body_ = body;
    headers_["Content-Length"] = std::to_string(body.size());
}

void HttpResponse::SetHeader(const std::string& key, const std::string& value)
{
    headers_[key] = value;
}

    // 清空上次的残留信息
void HttpResponse::Reset()
{
    statusCode_ = 200;
    status_ = "OK";

    body_.clear();
    headers_.clear();
    keepAlive_ = false;
}

// 设置text
void HttpResponse::SetText(const std::string& text)
{
    SetHeader("Content-Type", "text/plain");
    SetBody(text);
}
// 设置Html
void HttpResponse::SetHtml(const std::string& html)
{
    SetHeader("Content-Type", "text/html");
    SetBody(html);
}
// 设置keepalive
void HttpResponse::SetKeepAlive(bool keepAlive)
{
    keepAlive_ = keepAlive;
}



//------------------------private
    // 设置默认头部
void HttpResponse::BuildDefaultHeaders()
{
    if(headers_.count("Content-Type") == 0)
    {
        headers_["Content-Type"] = "text/plain";
    }

    headers_["Content-Length"] = std::to_string(body_.size());
    headers_["Connection"] = keepAlive_ ? "keep-alive" : "close";
}