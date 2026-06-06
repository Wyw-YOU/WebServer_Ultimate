#pragma once

#include <string>

class FileUtil
{
public:

    static bool ReadFile(const std::string& path, std::string& content);
};