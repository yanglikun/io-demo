//
//非阻塞测试。read会立即返回
// Created by Lincoln
//
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

int main() {
    //初始化 服务器端socket
    int listenFD = socket(AF_INET, SOCK_STREAM, 0);

    // 初始化socket的地址
    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(listenFD));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(8080);

    // 把socket绑定地址：指定的IP和端口
    int bindRet = bind(listenFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (0 != bindRet) {
        perror("bind失败");
        return 1;
    }

    //监听端口：修改lfd为被动socket，20是backlog的大小
    listen(listenFD, 20);

    printf("开始接收连接...\n");

    //初始化接收连接的套接字
    struct sockaddr_in clientAddr;
    socklen_t cliaddr_len = sizeof(clientAddr);

    //accept 阻塞接收连接
    int connfd = accept(listenFD, (struct sockaddr *) &clientAddr, &cliaddr_len);
    char str[INET_ADDRSTRLEN];
    printf("接收到连接 IP:%s,PORT:%d\n",
           inet_ntop(AF_INET, &clientAddr.sin_addr, str, sizeof(str)),
           ntohs(clientAddr.sin_port));

    //set 客户端连接为 non-blocking
    int flag = fcntl(connfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(connfd, F_SETFL, flag);

    while (1) {
        sleep(2);
        //连接成功,读数据
        char buf[1024] = ""; //buf变量属于userspace
        printf("读取数据开始.\n");
        int readBytes = read(connfd, buf, 1024);//读取数据
        if (readBytes == -1) {
            if (EWOULDBLOCK == errno || EAGAIN == errno) {//缓冲区没数据
                printf("缓冲区没有数据.continue. errno:%d\n", errno);
                continue;
            }
        }
        printf("读取数据结束,内容:%.*s , 字节:%d\n", readBytes, buf, readBytes);
        if (readBytes == 0) {//对方发送了FIN报文
            printf("读取的字节为0，客户端关闭了连接\n");
            break;
        }

    }
    printf("关闭连接 %s at PORT %d\n",
           inet_ntop(AF_INET, &clientAddr.sin_addr, str, sizeof(str)),
           ntohs(clientAddr.sin_port));
    close(connfd);
    return 0;
}
