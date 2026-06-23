#pragma once

#include <string>
#include <zlib.h>

class GzipUtil
{
public:
    static bool Compress(const std::string& body, std::string& out);
    static bool ShouldCompress(const std::string& contentType);
};
