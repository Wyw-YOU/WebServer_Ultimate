#pragma once

#include <string>
#include <unordered_map>

enum class HttpMethod
{
    GET,
    POST,
    PUT,
    DELETE_,
    UNKNOWN
};

class HttpRequest
{
public:
    // 解析HTTP请求// 后面优化完要删除（TODO
    bool Parse(const std::string& raw);

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

private:
    // 解析函数
    bool ParseHeaders(std::stringstream& ss);
    void ParseBody(std::stringstream& ss);
    bool ParseRequestLine(const std::string& line);

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
};