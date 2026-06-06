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

size_t Buffer::ReadableBytes() const
{
    return buffer_.size();
}

bool Buffer::Empty() const
{
    return buffer_.empty();
}