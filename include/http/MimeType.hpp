#pragma once

#include <string>

class MimeType
{
public:
    static std::string GetMime(const std::string& path);
};