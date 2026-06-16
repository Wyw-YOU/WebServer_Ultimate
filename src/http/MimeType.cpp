#include "http/MimeType.hpp"

#include <unordered_map>

std::string MimeType::GetMime(const std::string& path)
{
    static std::unordered_map<std::string, std::string> mimeMap =
    {
        {".html", "text/html"},
        {".css",  "text/css"},
        {".js",   "application/javascript"},
        {".json", "application/json"},

        {".png",  "image/png"},
        {".jpg",  "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif",  "image/gif"},
        {".ico",  "image/x-icon"},

        {".txt",  "text/plain"},
        {".svg",  "image/svg+xml"}
    };

    auto pos = path.rfind('.');
    if(pos == std::string::npos)
    {
        return "application/octet-stream";
    }

    std::string ext = path.substr(pos);

    auto it = mimeMap.find(ext);
    if(it != mimeMap.end())
    {
        return it->second;
    }

    return "application/octet-stream";
}