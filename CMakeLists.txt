cmake_minimum_required(VERSION 3.10)

project(WebServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 根据编译器设置警告标志
if(MSVC)
    # MSVC 编译器
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else()
    # GCC/Clang 编译器
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

file(GLOB_RECURSE SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_executable(
    server
    ${SRC_FILES}
)

# 根据平台链接库
if(WIN32)
    # Windows 平台：链接 mysqlclient（需要安装 MySQL Connector/C）
    # 注意：Windows 上需要手动指定 mysqlclient 的路径
    target_link_libraries(server)
else()
    # Linux/Unix 平台：链接 zlib 和 mysqlclient
    target_link_libraries(server z mysqlclient)
endif()