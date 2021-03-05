//
// Created by Lincoln
//
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <aio.h>
#include <stdlib.h>
#include <errno.h>

int main() {

    int fileFD = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fileFD) {
        perror("创建socket失败");
        return 1;
    }

    // 初始化sockaddr
    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof serverAddr);
    serverAddr.sin_family = AF_INET;
    short port = 8888;
    serverAddr.sin_port = htons(port); //正确的代码先注释掉
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // 建立连接
    int connectRet = connect(fileFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (-1 == connectRet) {
        perror("连接失败\n");
        return 1;
    }


    char *buffer = malloc(1024 * sizeof(char));
    memset(buffer, 0, sizeof(char) * 1024);

    struct aiocb ctlBlock;
    memset(&ctlBlock, 0, sizeof(ctlBlock));
    ctlBlock.aio_nbytes = 1024;// 读取的最大字节
    ctlBlock.aio_fildes = fileFD;
    ctlBlock.aio_offset = 0;
    ctlBlock.aio_buf = buffer;

    if (aio_read(&ctlBlock) == -1) {
        printf("aio_read失败\n");
        close(fileFD);
        free(buffer);
        exit(1);
    }

    while (aio_error(&ctlBlock) == EINPROGRESS) {
        printf("正在读取中\n");
    }
    int readBytes = aio_return(&ctlBlock);
    if (readBytes == -1) {
        printf("aio错误\n");
        close(fileFD);
        free(buffer);
        exit(1);
    }
    printf("读取的内容:%s", buffer);
    close(fileFD);
    free(buffer);
    return 0;
}

