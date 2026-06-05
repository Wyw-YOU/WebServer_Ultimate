#pragma once

#include <string>
#include <unordered_map>
#include <sstream>

/**
 * @brief HTTP响应对象
 */
class HttpResponse
{
public:
    HttpResponse()
        : statusCode_(200), status_("OK") {}

    void SetStatus(int code, const std::string& status)
    {
        statusCode_ = code;
        status_ = status;
    }

    void SetBody(const std::string& body)
    {
        body_ = body;
        headers_["Content-Length"] = std::to_string(body.size());
    }

    void SetHeader(const std::string& key, const std::string& value)
    {
        headers_[key] = value;
    }

    /**
     * @brief 将HttpResponse对象转换为HTTP协议格式的字符串
     */
    std::string ToString() const;

private:
    int statusCode_;
    std::string status_;
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
};