#include "util/GzipUtil.hpp"

bool GzipUtil::Compress(const std::string& body, std::string& out)
{
    z_stream zs{};
    // windowBits = 15 + 16 → gzip 格式
    if(deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                    15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        return false;
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(body.data()));
    zs.avail_in = static_cast<uInt>(body.size());

    size_t outSize = body.size();
    out.resize(outSize);

    zs.next_out = reinterpret_cast<Bytef*>(&out[0]);
    zs.avail_out = static_cast<uInt>(outSize);

    int ret = deflate(&zs, Z_FINISH);
    if(ret == Z_STREAM_END)
    {
        out.resize(zs.total_out);
        deflateEnd(&zs);
        return true;
    }

    // 缓冲不够，扩容后重试
    if(ret == Z_OK || ret == Z_BUF_ERROR)
    {
        outSize = zs.total_out + body.size() / 2 + 128;
        out.resize(outSize);
        zs.next_out = reinterpret_cast<Bytef*>(&out[zs.total_out]);
        zs.avail_out = static_cast<uInt>(outSize - zs.total_out);

        ret = deflate(&zs, Z_FINISH);
        if(ret == Z_STREAM_END)
        {
            out.resize(zs.total_out);
            deflateEnd(&zs);
            return true;
        }
    }

    deflateEnd(&zs);
    return false;
}

bool GzipUtil::ShouldCompress(const std::string& contentType)
{
    // 只压缩文本类型
    if(contentType.find("text/") != std::string::npos)
        return true;
    if(contentType.find("application/json") != std::string::npos)
        return true;
    if(contentType.find("application/javascript") != std::string::npos)
        return true;
    if(contentType.find("application/xml") != std::string::npos)
        return true;
    return false;
}
