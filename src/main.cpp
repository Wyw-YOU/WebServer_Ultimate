#include "Server.hpp"
#include "thread/ThreadPool.hpp"

#include <iostream>
#include <chrono>
#include <thread>

int main(int argc,char* argv[])
{
    if(argc >= 2 && std::string(argv[1]) == "test_pool")
    {
        // 测试线程池
        ThreadPool pool(4);

        for(int i = 0; i < 20; i++)
        {
            pool.AddTask([i]()
            {
                std::cout
                    << "Task "
                    << i
                    << " thread="
                    << std::this_thread::get_id()
                    << std::endl;

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
        }

        // 给线程池足够时间处理任务
        std::this_thread::sleep_for(std::chrono::seconds(3));

        return 0;
    }

    // 原本的Server启动
    if(argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <port> [resource_dir]" << std::endl;
        return 0;
    }

    int port = atoi(argv[1]);
    std::string resourceDir = (argc >= 3) ? argv[2] : "../resources";

    Server server(port, resourceDir);
    server.Start();

    return 0;
}