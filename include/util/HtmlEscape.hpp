#pragma once

#include <string>
#include <cstdio>

class HtmlEscape
{
public:
    // 转义 HTML 特殊字符（用于 HTML 上下文）
    static std::string EscapeHtml(const std::string& input)
    {
        std::string output;
        output.reserve(input.size() + input.size() / 10);

        for(char c : input)
        {
            switch(c)
            {
                case '&':  output += "&amp;";  break;
                case '<':  output += "&lt;";   break;
                case '>':  output += "&gt;";   break;
                case '"':  output += "&quot;"; break;
                case '\'': output += "&#39;";  break;
                default:   output += c;
            }
        }

        return output;
    }

    // 转义 JavaScript 字符串字面量（用于 JS 字符串上下文）
    static std::string EscapeJsString(const std::string& input)
    {
        std::string output;
        output.reserve(input.size() + input.size() / 5);

        for(char c : input)
        {
            switch(c)
            {
                case '\\': output += "\\\\"; break;
                case '\'': output += "\\'";  break;
                case '"':  output += "\\\""; break;
                case '\n': output += "\\n";  break;
                case '\r': output += "\\r";  break;
                case '\t': output += "\\t";  break;
                case '/':  output += "\\/";  break;
                default:
                {
                    // 转义控制字符（ASCII < 0x20）
                    if(static_cast<unsigned char>(c) < 0x20)
                    {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\x%02x",
                                static_cast<unsigned char>(c));
                        output += buf;
                    }
                    else
                    {
                        output += c;
                    }
                }
            }
        }

        return output;
    }
};
