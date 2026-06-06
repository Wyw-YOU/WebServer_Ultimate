#include "util/FileUtil.hpp"

#include <fstream>
#include <sstream>

bool FileUtil::ReadFile(const std::string& path,std::string& content)
{
    std::ifstream file(path);
    if(!file.is_open())
    {
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    content = ss.str();

    return true;
}