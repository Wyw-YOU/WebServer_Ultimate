#include "buffer/Buffer.hpp"

Buffer::Buffer()
{ }

void Buffer::Append(const char* data, size_t len)
{
    buffer_.append(data, len);
}

std::string Buffer::RetrieveAll()
{
    std::string data = buffer_;
    buffer_.clear();

    return data;
}

std::string Buffer::Retrieve(size_t len)
{
    if(len > buffer_.size())
        len = buffer_.size();

    std::string data = buffer_.substr(0, len);
    buffer_ = buffer_.substr(len);

    return data;
}

const std::string& Buffer::Peek() const
{
    return buffer_;
}

size_t Buffer::ReadableBytes() const
{
    return buffer_.size();
}

bool Buffer::Empty() const
{
    return buffer_.empty();
}