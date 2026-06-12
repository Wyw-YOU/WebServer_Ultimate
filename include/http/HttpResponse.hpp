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
        : statusCode_(200)
        , keepAlive_(false)
        , status_("OK") 
        { }

    void SetStatus(int code, const std::string& status);
    void SetBody(const std::string& body);
    void SetHeader(const std::string& key, const std::string& value);
    
    // 清空上次的残留信息
    void Reset();
    void SetText(const std::string& text);
    void SetHtml(const std::string& html);
    void SetKeepAlive(bool keepAlive);

    /**
     * @brief 将HttpResponse对象转换为HTTP协议格式的字符串
     */
    std::string ToString() const;

    // 设置默认头部
    void BuildDefaultHeaders();

private:
    int statusCode_;
    bool keepAlive_;
    std::string status_;
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
};