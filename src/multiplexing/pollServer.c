//
// Created by Lincoln
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

#define MAX_CONNECTION 1024

/**
 * 没有1024限制
 * poll的时候需要传递到内核
 *需要遍历所有的客户端,来确定哪些发生了变化,大量并发，少量活跃的时候，效率不高
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
    //初始化 服务器端socket
    int listenFD = socket(AF_INET, SOCK_STREAM, 0);

    // 初始化socket的地址
    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(listenFD));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(8080);

    //设置 服务器端fd为 non-blocking
    int flag = fcntl(listenFD, F_GETFL);
    flag |= O_NONBLOCK;
    int ret = fcntl(listenFD, F_SETFL, flag);
    if (ret < 0) {
        perror("设置listenFD非阻塞失败");
        exit(1);
    }

    // 把socket绑定地址：指定的IP和端口
    int bindRet = bind(listenFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (0 != bindRet) {
        perror("bind失败");
        return 1;
    }

    //监听端口：修改lfd为被动socket，20是backlog的大小
    listen(listenFD, 20);

    printf("poll.开始接收连接....listenFD:%d\n", listenFD);

    struct pollfd client[MAX_CONNECTION]; //最大打开的连接
    for (int i = 1; i < MAX_CONNECTION; i++) {
        client[i].fd = -1;
    }

    client[0].fd = listenFD;
    client[0].events = POLLIN;
    int maxIdx = 0; //client数组的最大下标


    while (1) {
        int changeFDNum = poll(client, maxIdx + 1, -1);
        printf("poll触发,变化的FD个数:%d\n", changeFDNum);
        if (changeFDNum < 0) { //变化的个数小于0
            perror("poll error");
            continue;
        }
        for (int i = 0; i <= maxIdx; i++) {
            if (client[i].fd < 0) { //这个位置没有描述符
                continue;
            }
            if (client[i].fd == listenFD && (client[i].revents & POLLIN)) { //监听套接字，有可读事件
                // 接收客户端连接
                struct sockaddr_in cliaddr;
                socklen_t cliaddr_len = sizeof(cliaddr);
                int connfd = accept(listenFD, (struct sockaddr *) &cliaddr, &cliaddr_len);
                char str[INET_ADDRSTRLEN];
                printf("接收到连接 IP:%s,PORT:%d,curFD:%d\n",
                       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                       ntohs(cliaddr.sin_port), connfd);
                int curIndx = 1;
                for (curIndx = 1; curIndx < MAX_CONNECTION; curIndx++) {
                    if (client[curIndx].fd < 0) { //找到空闲的位置
                        break;
                    }
                }
                if (curIndx == MAX_CONNECTION) {
                    printf("超过最大连接数\n");
                    close(connfd);
                    break;
                }

                //设置 客户端连接为 non-blocking
                int flagOfConnfd = fcntl(connfd, F_GETFL);
                flagOfConnfd |= O_NONBLOCK;
                ret = fcntl(connfd, F_SETFL, flagOfConnfd);
                if (ret < 0) {
                    perror("设置connFD非阻塞失败");
                    exit(1);
                }
                client[curIndx].fd = connfd;
                client[curIndx].events = POLLIN;
                if (curIndx > maxIdx) {
                    maxIdx = curIndx;
                }
            } else if (client[i].revents & POLLIN) { // 非连接描述符的可读事件
                int curFD = client[i].fd;
                char buf[1024] = "";
                int readBytes = read(curFD, buf, 1024);
                if (readBytes < 0) {
                    if (EWOULDBLOCK == errno || EAGAIN == errno) {//非阻塞IO，缓冲区没数据
                        printf("缓冲区没有数据.continue. errno:%d\n", errno);
                        continue;
                    }
                } else if (readBytes == 0) {//客户端关闭连接
                    client[i].fd = -1;
                    close(curFD);
                    printf("连接关闭.curFD:%d\n", curFD);
                } else {//接收到数据，写入客户端
                    write(curFD, buf, readBytes);
                }
            }
        }

    }
    return 0;
}

#pragma clang diagnostic pop