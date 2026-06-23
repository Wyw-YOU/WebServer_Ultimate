#pragma once

#include <string>
#include <unordered_map>

inline std::string UrlDecode(const std::string& src)
{
    std::string result;
    result.reserve(src.size());

    for(size_t i = 0; i < src.size(); ++i)
    {
        if(src[i] == '%' && i + 2 < src.size())
        {
            auto hexToChar = [](char c) -> int {
                if(c >= '0' && c <= '9') return c - '0';
                if(c >= 'A' && c <= 'F') return c - 'A' + 10;
                if(c >= 'a' && c <= 'f') return c - 'a' + 10;
                return -1;
            };
            int hi = hexToChar(src[i + 1]);
            int lo = hexToChar(src[i + 2]);
            if(hi >= 0 && lo >= 0)
            {
                result += static_cast<char>(hi * 16 + lo);
                i += 2;
                continue;
            }
        }
        else if(src[i] == '+')
        {
            result += ' ';
            continue;
        }
        result += src[i];
    }
    return result;
}

inline std::unordered_map<std::string, std::string> ParseFormBody(const std::string& body)
{
    std::unordered_map<std::string, std::string> params;
    size_t pos = 0;

    while(pos < body.size())
    {
        size_t amp = body.find('&', pos);
        if(amp == std::string::npos)
            amp = body.size();

        std::string pair = body.substr(pos, amp - pos);
        size_t eq = pair.find('=');
        if(eq != std::string::npos)
        {
            std::string key = UrlDecode(pair.substr(0, eq));
            std::string value = UrlDecode(pair.substr(eq + 1));
            params[key] = value;
        }

        pos = amp + 1;
    }
    return params;
}
