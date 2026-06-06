#include "Server.hpp"

#include <iostream>

int main(int argc,char* argv[])
{
    if(argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <port> [resource_dir]" << std::endl;
        return 0;
    }

    int port = atoi(argv[1]);
    std::string resourceDir = (argc >= 3) ? argv[2] : "resources";

    Server server(port, resourceDir);
    server.Start();

    return 0;
}