#pragma once
#include <memory>
#include "buffer/Buffer.hpp"

class Connection;

struct HttpTask
{
    std::shared_ptr<Connection> conn;
    Buffer requestBuffer;
};


// （IO → Worker）
// 职责:
// 只做“请求搬运”
// 不允许解析 / IO操作
// 不允许调用 Connection 业务函数