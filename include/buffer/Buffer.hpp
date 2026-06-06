#pragma once

#include <string>

/**
 * @brief 网络缓冲区
 */
class Buffer
{
public:
    Buffer();

    /**
     * @brief 追加数据
     */
    void Append(const char* data, size_t len);

    /**
     * @brief 获取所有数据（清空缓冲区）
     */
    std::string RetrieveAll();
    std::string Retrieve(size_t len);

    /**
     * @brief 查看数据但不清空
     */
    const std::string& Peek() const;

    /**
     * @brief 当前数据长度
     */
    size_t ReadableBytes() const;

    /**
     * @brief 是否为空
     */
    bool Empty() const;

private:
    std::string buffer_;
};