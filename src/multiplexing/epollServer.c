//
// Created by Lincoln
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main() {
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
    if (listen(listenFD, 20) < 0) {
        perror("listen失败");
        exit(1);
    }

    printf("epoll.开始接收连接....listenFD:%d\n", listenFD);

    //创建epoll实例,size:需要内核检测的描述符数量， Linux 2.6.8 参数被忽略,内核会动态分配
    int epollFD = epoll_create(1);
    if (epollFD == -1) {
        perror("创建epoll实例失败\n");
        exit(1);
    }

    const int MAX_EVENT = 1024;
    struct epoll_event event;
    struct epoll_event *events = malloc(sizeof(event) * MAX_EVENT);

    event.data.fd = listenFD;
    event.events = EPOLLIN;

    //添加‘监听套接字‘到epoll实例
    int res = epoll_ctl(epollFD, EPOLL_CTL_ADD, listenFD, &event);
    if (res == -1) {
        perror("添加listenFD到epoll失败");
        exit(1);
    }

    while (1) {
        //阻塞等待
        int changeNum = epoll_wait(epollFD, events, MAX_EVENT, -1);
        printf("epll_wait返回,变化的描述符数:%d\n", changeNum);
        for (int i = 0; i < changeNum; i++) {
            if (!(events[i].events & EPOLLIN)) { //只处理可读事件
                continue;
            }
            if (events[i].data.fd == listenFD) { //监听套接字可读，说明有新的连接
                // 接收客户端连接
                struct sockaddr_in cliaddr;
                socklen_t cliaddr_len = sizeof(cliaddr);
                int connfd = accept(listenFD, (struct sockaddr *) &cliaddr, &cliaddr_len);
                if (connfd < 0) {
                    perror("accept失败");
                    continue;
                }
                char str[INET_ADDRSTRLEN];
                printf("接收到连接 IP:%s,PORT:%d,curFD:%d\n",
                       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                       ntohs(cliaddr.sin_port), connfd);
                //设置 客户端连接为 non-blocking
                int flagOfConnfd = fcntl(connfd, F_GETFL);
                flagOfConnfd |= O_NONBLOCK;
                ret = fcntl(connfd, F_SETFL, flagOfConnfd);
                if (ret < 0) {
                    perror("设置connFD非阻塞失败");
                }
                event.data.fd = connfd;
                event.events = EPOLLIN;
                if (epoll_ctl(epollFD, EPOLL_CTL_ADD, connfd, &event) < 0) {
                    perror("添加connFD到epoll失败");
                    close(connfd);
                    continue;
                }
            } else {
                int curFD = events[i].data.fd;
                char buf[1024] = "";
                int readBytes = read(curFD, buf, 1024);
                if (readBytes < 0) {
                    if (EWOULDBLOCK == errno || EAGAIN == errno) {//非阻塞IO，缓冲区没数据
                        printf("缓冲区没有数据.continue. errno:%d\n", errno);
                        continue;
                    } else {
                        printf("数据读取失败,关闭连接\n");
                        if (epoll_ctl(epollFD, EPOLL_CTL_DEL, curFD, NULL) < 0) {
                            perror("从epoll删除FD失败");
                        }
                        close(curFD);
                        printf("连接关闭.curFD:%d\n", curFD);
                    }
                } else if (readBytes == 0) {//客户端关闭连接
                    if (epoll_ctl(epollFD, EPOLL_CTL_DEL, curFD, NULL) < 0) {
                        perror("从epoll删除FD失败");
                    }
                    close(curFD);
                    printf("连接关闭.curFD:%d\n", curFD);
                } else {//接收到数据，写入客户端
                    write(curFD, buf, readBytes);
                }
            }
        }
    }

    free(events);
    close(listenFD);
    return 0;
}

#pragma clang diagnostic pop