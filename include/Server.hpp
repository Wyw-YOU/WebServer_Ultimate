#pragma once

/**
 * @brief WebServer类
 */
class Server
{
public:

    /**
     * @brief 构造函数
     *
     * @param port 监听端口
     */
    explicit Server(int port);

    /**
     * @brief 启动服务器
     */
    void Start();

private:

    int port_;
};