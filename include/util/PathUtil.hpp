#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <climits>
#include <unistd.h>
#include <sys/stat.h>
#include "util/UrlDecode.hpp"

class PathUtil
{
public:
    // 安全路径检查，返回规范化后的相对路径
    // 非法路径返回空字符串
    static std::string SafePath(const std::string& resourceDir,
                                const std::string& requestPath)
    {
        // 1. 检查空字节（空字节注入攻击）
        if(requestPath.find('\0') != std::string::npos)
            return "";

        // 2. URL 解码（防止 %2e%2e 绕过）
        std::string decoded = UrlDecode(requestPath);

        // 3. 再次检查空字节（解码后可能出现）
        if(decoded.find('\0') != std::string::npos)
            return "";

        // 4. 规范化路径（移除连续 /、解析 . 和 ..）
        std::string normalized = NormalizePath(decoded);

        // 5. 规范化后再次检查 ..
        if(normalized.find("..") != std::string::npos)
            return "";

        // 6. 确保路径以 / 开头
        if(!normalized.empty() && normalized[0] != '/')
            normalized = "/" + normalized;

        // 7. 拼接完整路径
        std::string fullPath = resourceDir + normalized;

        // 8. 获取 resourceDir 的绝对路径
        std::string absResourceDir = GetAbsolutePath(resourceDir);
        if(absResourceDir.empty())
            return "";

        // 9. 获取完整路径的绝对路径
        std::string absFullPath = GetAbsolutePath(fullPath);
        if(absFullPath.empty())
            return "";

        // 10. 确保 absFullPath 以 absResourceDir 开头
        //     并且下一个是 / 或字符串结束（防止 /var/www-public 被 /var/www 匹配）
        if(absFullPath.find(absResourceDir) != 0)
            return "";

        if(absFullPath.length() > absResourceDir.length())
        {
            if(absFullPath[absResourceDir.length()] != '/')
                return "";
        }

        // 11. 返回规范化后的相对路径
        return normalized;
    }

private:
    // 规范化路径：移除连续 /、解析 . 和 ..
    static std::string NormalizePath(const std::string& path)
    {
        if(path.empty())
            return "/";

        std::vector<std::string> components;
        std::istringstream stream(path);
        std::string component;

        // 按 / 分割路径
        while(std::getline(stream, component, '/'))
        {
            if(component.empty() || component == ".")
            {
                // 跳过空组件和当前目录 .
                continue;
            }
            else if(component == "..")
            {
                // 上级目录，弹出最后一个组件（如果有）
                if(!components.empty())
                    components.pop_back();
            }
            else
            {
                // 普通组件，添加到列表
                components.push_back(component);
            }
        }

        // 重建路径
        std::string result;
        for(const auto& comp : components)
        {
            result += "/" + comp;
        }

        return result.empty() ? "/" : result;
    }

    // 获取绝对路径
    static std::string GetAbsolutePath(const std::string& path)
    {
        char resolved[PATH_MAX];
        if(realpath(path.c_str(), resolved) == nullptr)
            return "";
        return std::string(resolved);
    }
};
