cmake_minimum_required(VERSION 3.17)
project(io_demo C)

set(CMAKE_C_STANDARD 99)
IF (UNIX)
    add_executable(io_demo main.c)
    add_executable(demo1 src/blockServer.c)
    add_executable(nonblock src/nonblockServer.c)
    add_executable(select src/multiplexing/selectServer.c)
    add_executable(poll src/multiplexing/pollServer.c)
    add_executable(epoll src/multiplexing/epollServer.c)
ENDIF ()

IF (WIN32)

ENDIF ()