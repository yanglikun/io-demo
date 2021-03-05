//
// Created by Lincoln
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <aio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

int main() {
    //读取/etc/hosts文件
    int fileFD = open("/etc/hosts", O_RDONLY, 0);
    if (fileFD < 0) {
        perror("打开文件失败");
        exit(1);
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

