#include "util/FileUtil.hpp"

bool FileUtil::Exists(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool FileUtil::IsRegularFile(const std::string& path)
{
    struct stat st;
    if(stat(path.c_str(), &st) < 0)
    {
        return false;
    }

    return S_ISREG(st.st_mode);
}

bool FileUtil::ReadFile(const std::string& path, std::string& content)
{
    std::ifstream file(path, std::ios::binary);

    if(!file.is_open())
    {
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();

    content = ss.str();

    return true;
}


size_t FileUtil::FileSize(const std::string& path)
{
    struct stat st;
    if(stat(path.c_str(), &st) < 0)
    {
        return 0;
    }

    return st.st_size;
}