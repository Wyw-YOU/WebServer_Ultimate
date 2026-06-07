#include "buffer/Buffer.hpp"

Buffer::Buffer(size_t initSize)
    : buffer_(initSize, '\0'),
      readPos_(0),
      writePos_(0)
{ }

void Buffer::MakeSpace(size_t len)
{
    size_t writable = WritableBytes();
    if(writable >= len) // 已经有足够空间了
    {
        return;
    }
    else if(readPos_ + writable >= len)
    {
        // 将未读数据移动到缓冲区前面
        size_t readable = ReadableBytes();
        std::copy(buffer_.begin() + readPos_, buffer_.begin() + writePos_, buffer_.begin());
        readPos_ = 0;
        writePos_ = readable;
        writable = WritableBytes();

        if(writable >= len)
            return;
    }
    // 空间还是不足，则扩展缓冲区
    size_t newSize = std::max(buffer_.size() * 2, writePos_ + len);
    buffer_.resize(newSize);
}

void Buffer::Append(const char* data, size_t len)
{
    MakeSpace(len);
    // 拷贝到写位置
    std::memcpy(&buffer_[writePos_], data, len);
    writePos_ += len;
}

// 获取所有数据（清空缓冲区）
std::string Buffer::RetrieveAll()
{
    std::string data(buffer_.data() + readPos_, ReadableBytes());
    readPos_ = writePos_ = 0;

    return data;
}
// 获取指定长度数据（清空缓冲区）
std::string Buffer::Retrieve(size_t len)
{
    size_t readable = ReadableBytes();
    size_t take = std::min(len, readable);
    std::string data(buffer_.data() + readPos_, take);
    readPos_ += take;
    if(readPos_ == writePos_)
        readPos_ = writePos_ = 0;

    return data;
}

// 查看数据但不清空
const std::string& Buffer::Peek() const
{
    return buffer_;
}

const char* Buffer::ReadBegin() const
{
    return buffer_.data() + readPos_;
}

std::pair<const char*, size_t> Buffer::PeekReadable() const
{
    return {buffer_.data() + readPos_, ReadableBytes()};
}

size_t Buffer::ReadableBytes() const
{
    return writePos_ - readPos_;
}

size_t Buffer::WritableBytes() const
{
    return buffer_.size() - writePos_;
}

bool Buffer::Empty() const
{
    return ReadableBytes() == 0;
}