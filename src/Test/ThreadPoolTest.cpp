#include "thread/ThreadPool.hpp"

ThreadPool pool(4);

for(int i=0;i<20;i++)
{
    pool.AddTask(
        [i]
        {
            std::cout
                << "Task "
                << i
                << std::endl;
        });
}