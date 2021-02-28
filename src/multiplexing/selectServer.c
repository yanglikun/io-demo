//
// Created by Lincoln
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

/**
 * 缺点：
 * 1、最大连接限制:1024，可以查看fd_set源码的实现
 * 2、每次select都要复制数据到内核(这个是最大的性能开销)
 * 3、需要循环所有的client来判断是否发生变化，大量并发，少量活跃的时候，效率不高
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

    printf("select.开始接收连接....listenFD:%d\n", listenFD);

    fd_set allFDs;//所有需要监听的fd集合
    FD_ZERO(&allFDs);
    FD_SET(listenFD, &allFDs);
    int maxFD = listenFD;
    while (1) {
        //复制allFDS到readFDS，因为select会修改入参
        fd_set readFDs = allFDs;
        //当监听到变化,readFDs里面变化的位为1，未变化的为0
        int changeFDNum = select(maxFD + 1, &readFDs, NULL, NULL, NULL);
        printf("select触发,变化的FD个数:%d\n", changeFDNum);
        if (changeFDNum < 0) { //变化的个数小于0
            perror("select error");
            continue;
        }

        //遍历是那个FD发生了变化
        for (int curFD = 0; curFD < maxFD + 1; curFD++) {
            if (!FD_ISSET(curFD, &readFDs)) {//这个位置上没有变化
                continue;
            }
            if (curFD == listenFD) {//服务器端socket可读，说明有新的连接
                //接收客户端连接
                struct sockaddr_in cliaddr;
                socklen_t cliaddr_len = sizeof(cliaddr);
                int connfd = accept(listenFD, (struct sockaddr *) &cliaddr, &cliaddr_len);

                //设置 客户端连接为 non-blocking
                int flagOfConnfd = fcntl(connfd, F_GETFL);
                flagOfConnfd |= O_NONBLOCK;
                ret = fcntl(connfd, F_SETFL, flagOfConnfd);
                if (ret < 0) {
                    perror("设置connFD非阻塞失败");
                    exit(1);
                }

                //加入到select监听集合
                FD_SET(connfd, &allFDs);
                if (maxFD < connfd) {
                    maxFD = connfd;//设置最大的FD
                }
                char str[INET_ADDRSTRLEN];
                printf("接收到连接 IP:%s,PORT:%d,curFD:%d\n",
                       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                       ntohs(cliaddr.sin_port), connfd);
            } else { //非listenFD，说明有数据可读
                char buf[1024] = "";
                int readBytes = read(curFD, buf, 1024);
                if (readBytes < 0) {
                    if (EWOULDBLOCK == errno || EAGAIN == errno) {//非阻塞IO，缓冲区没数据
                        printf("缓冲区没有数据.continue. errno:%d\n", errno);
                        continue;
                    }
                } else if (readBytes == 0) {//客户端关闭连接
                    FD_CLR(curFD, &allFDs);
                    close(curFD);
                    printf("连接关闭.curFD:%d\n", curFD);
                } else {//接收到数据，写入客户端
                    write(curFD, buf, readBytes);
                }
            }

        }
    }
    close(listenFD);
    return 0;
}

#pragma clang diagnostic pop