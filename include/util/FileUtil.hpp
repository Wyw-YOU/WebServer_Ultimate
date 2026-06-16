#pragma once

#include <sys/stat.h>
#include <string>
#include <fstream>
#include <sstream>

class FileUtil
{
public:
    static bool Exists(const std::string& path);
    static bool IsRegularFile(const std::string& path);
    static bool ReadFile(const std::string& path, std::string& content);

    // 文件大小检查
    static size_t FileSize(const std::string& path);
};