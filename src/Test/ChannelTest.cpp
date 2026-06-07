// #include "net/Channel.hpp"

// #include <iostream>
// #include <sys/epoll.h>

// int main()
// {
//     Channel channel(10);

//     channel.SetReadCallback(
//         []()
//         {
//             std::cout
//                 << "read callback"
//                 << std::endl;
//         });

//     channel.SetWriteCallback(
//         []()
//         {
//             std::cout
//                 << "write callback"
//                 << std::endl;
//         });

//     channel.SetCloseCallback(
//         []()
//         {
//             std::cout
//                 << "close callback"
//                 << std::endl;
//         });

//     std::cout
//         << "==== EPOLLIN Test ===="
//         << std::endl;

//     channel.SetRevents(EPOLLIN);

//     channel.HandleEvent();

//     std::cout
//         << "==== EPOLLOUT Test ===="
//         << std::endl;

//     channel.SetRevents(EPOLLOUT);

//     channel.HandleEvent();

//     std::cout
//         << "==== EPOLLERR Test ===="
//         << std::endl;

//     channel.SetRevents(EPOLLERR);

//     channel.HandleEvent();

//     return 0;
// }