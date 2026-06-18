#pragma once

#include <string>
#include <unordered_map>

enum class HttpMethod
{
    GET,
    POST,
    PUT,
    DELETE_,
    HEAD,
    UNKNOWN
};

class HttpRequest
{
public:
    // 清空上次的残留信息
    void Reset();

    const std::string& Method() const;
    const std::string& Path() const;
    const std::string& Version() const;

    std::string GetHeader(const std::string& key) const;

    bool IsKeepAlive() const;

    // 添加头
    bool AddHeader(const std::string& line);
    // body追加
    void AppendBody(const std::string& body);
    // 获取body
    const std::string& Body() const;

    void SetBody(const std::string& body);
    void SetHeader(const std::string& key, const std::string& value);
    bool ParseStartLine(const std::string& line);
    // 内容长度
    size_t ContentLength() const;
    // 请求类型
    HttpMethod MethodType() const;

private:
    bool ParseRequestLine(const std::string& line);

    std::string method_;
    std::string path_;
    std::string version_;
    std::string body_;
    HttpMethod methodType_ = HttpMethod::UNKNOWN;
    std::unordered_map<std::string, std::string> headers_;
};