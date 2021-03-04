cmake_minimum_required(VERSION 3.17)
project(io_demo C)

#如果乱码，可以尝试放开注释
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=gbk")
#set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=gbk")

set(CMAKE_C_STANDARD 99)
IF (UNIX)
    add_executable(io_demo main.c)
    add_executable(demo1 src/blockServer.c)
    add_executable(nonblock src/nonblockServer.c)
    add_executable(select src/multiplexing/selectServer.c)
    add_executable(poll src/multiplexing/pollServer.c)
    add_executable(epoll src/multiplexing/epollServer.c)
    add_executable(serverET src/multiplexing/epollETLT.c)
ENDIF ()

IF (WIN32)
    add_executable(iocp src/multiplexing/IOCPServer.c)
ENDIF ()