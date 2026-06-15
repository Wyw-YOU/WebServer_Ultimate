#include "http/HttpContext.hpp"

namespace
{
    bool ReadLine(Buffer& buffer, std::string& line)
    {
        const char* crlf = buffer.FindCRLF();
        if(crlf == nullptr)
        {
            return false;
        }
    
        const char* begin = buffer.ReadBegin();
        size_t len = crlf - begin;
        line.assign(begin, len);
        buffer.Consume(len + 2);
    
        return true;
    }
}

    // 解析请求
ParseResult HttpContext::ParseRequest(Buffer& buffer)
{
    while(true)
    {
        switch(state_)
        {
            case ParseState::REQUEST_LINE:
            {
                ParseResult result = ParseRequestLine(buffer);
                if(result != ParseResult::Complete)
                {
                    return result;
                }
                break;
            }
            case ParseState::HEADERS:
            {
                ParseResult result = ParseHeaders(buffer);
                if(result != ParseResult::Complete)
                {
                    return result;
                }
                break;
            }
            case ParseState::BODY:
            {
                ParseResult result = ParseBody(buffer);
                if(result != ParseResult::Complete)
                {
                    return result;
                }
                break;
            }
            case ParseState::FINISH:
            {
                complete_ = true;
                return ParseResult::Complete;
            }
            default:
            {
                return ParseResult::Error;
            }
        }
    }
}

    // 判断包是否完整
bool HttpContext::IsComplete() const
{
    return complete_;
}

    // 获取请求
HttpRequest& HttpContext::Request()
{
    return request_;
}

    // 清除请求的缓存ss
void HttpContext::Reset()
{
    request_.Reset();
    state_ = ParseState::REQUEST_LINE;
    contentLength_ = 0;
    complete_ = false;
}

ParseState HttpContext::GetState() const
{
    return state_;
}




//         private:
ParseResult HttpContext::ParseRequestLine(Buffer& buffer)
{
    std::string line;
    
    if(!ReadLine(buffer, line))
    {
        return ParseResult::Incomplete;
    }
    if(!request_.ParseStartLine(line))
    {
        return ParseResult::Error;
    }
    state_ = ParseState::HEADERS;

    return ParseResult::Complete;
}

ParseResult HttpContext::ParseHeaders(Buffer& buffer)
{
    while(true)
    {
        std::string line;
    
        if(!ReadLine(buffer, line))
        {
            return ParseResult::Incomplete;
        }
    
        if(line.empty())
        {
            if(contentLength_ > 0)
            {
                state_ = ParseState::BODY;
            }
            else
            {
                state_ = ParseState::FINISH;
            }
    
            return ParseResult::Complete;
        }
    
        if(!ParseHeaderLine(line))
        {
            return ParseResult::Error;
        }
    }
}

bool HttpContext::ParseHeaderLine(const std::string& line)
{
    auto pos = line.find(':');
    if(pos == std::string::npos)
    {
        return false;
    }

    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    size_t start = value.find_first_not_of(' ');
    if(start != std::string::npos)
    {
        value = value.substr(start);
    }

    request_.SetHeader(key, value);
    if(key == "content-length")
    {
        try
        {
            contentLength_ = std::stoul(value);
        }
        catch(...)
        {
            return false;
        }
    }

    return true;
}

ParseResult HttpContext::ParseBody(Buffer& buffer)
{
    if(buffer.ReadableBytes() < contentLength_)
    {
        return ParseResult::Incomplete;
    }

    auto readable = buffer.PeekReadable();
    request_.SetBody(std::string(readable.first, contentLength_));

    buffer.Consume(contentLength_);
    state_ = ParseState::FINISH;

    return ParseResult::Complete;
}