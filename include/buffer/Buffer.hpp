#pragma once

#include <cstring>
#include <string>
#include <algorithm>

/**
 * @brief 网络缓冲区
 */
class Buffer
{
public:
    explicit Buffer(size_t initSize = 1024);

    // 追加数据
    void Append(const char* data, size_t len);

    // 获取所有数据（清空缓冲区）
    std::string RetrieveAll();
    std::string Retrieve(size_t len);

    // 查看数据但不清空（返回整个内部 buffer，可能含已读旧数据）
    const std::string& Peek() const;

    // 返回可读数据的指针和长度（零拷贝）
    const char* ReadBegin() const;
    std::pair<const char*, size_t> PeekReadable() const;

    // @brief 当前数据长度
    size_t ReadableBytes() const;
    size_t WritableBytes() const;


    // 是否为空

    bool Empty() const;

private:
    std::string buffer_;
    size_t readPos_;
    size_t writePos_;

    // 确保有足够空间写入数据
    void MakeSpace(size_t len);
};