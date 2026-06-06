#pragma once

#include <string>
#include <unordered_map>

class HttpRequest
{
public:
    // 解析HTTP请求
    bool Parse(const std::string& raw);

    const std::string& Method() const;
    const std::string& Path() const;
    const std::string& Version() const;

    std::string GetHeader(const std::string& key) const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
};