#include "Server.hpp"

#include <iostream>

int main(int argc,char* argv[])
{
    if(argc != 2)
    {
        std::cout << "Usage: " << argv[0]  << " <port>" << std::endl;
        return 0;
    }

    int port = atoi(argv[1]);

    Server server(port);

    server.Start();

    return 0;
}