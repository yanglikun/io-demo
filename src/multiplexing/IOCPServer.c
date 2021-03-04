//windows平台下运行
// Created by Lincoln
//
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include "DateUtil.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define PORT 8080
#define BUF_SIZE 1024
// IOCP本身不区分输入完成和输出完成状态
#define  READ 3
#define WRITE 5

// 客户端socket信息
typedef struct {
    SOCKET clientSocket;
    SOCKADDR_IN clientAddr;
} ClientData;

//IO缓冲区
typedef struct {
    OVERLAPPED overlapped;
    WSABUF wsabuf;
    char buffer[BUF_SIZE];
    int rwMode;//读或写
    unsigned long receiveBytes;
    unsigned long sendBytes;
} IOData;

unsigned int WINAPI
echoThead(LPVOID
completionPortIO);

int main() {
    setbuf(stdout, 0);//解决debug下,printf不实时输出问题

    //初始化winsock版本
    struct WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("设置socket版本错误");
        exit(1);
    }
    //创建CP对象，已完成的IO消息会注册到CP对象
    HANDLE compPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    //创建线程
    _beginthreadex(NULL, 0, echoThead, compPort, 0, NULL);
    //创建serverSocket
    SOCKET serverSocket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    //绑定地址和端口
    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);
    if (bind(serverSocket, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("绑定端口失败");
        exit(1);
    }
    //开启监听模式
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        perror("监听失败");
        exit(1);
    }
    printf("监听成功，等待客户端连接.port:%d\n", PORT);

    DWORD flags = 0;
    //主线程循环accept新连接
    while (1) {
        //accept新连接，并保存连接信息到 clientDataPt
        ClientData *clientDataPt = malloc(sizeof(ClientData));
        memset(clientDataPt, 0, sizeof(ClientData));
        int clientAddrLen = sizeof(clientDataPt->clientAddr);
        clientDataPt->clientSocket = accept(serverSocket, (struct sockaddr *) &clientDataPt->clientAddr,
                                            &clientAddrLen);
        printf("接收到客户端连接,客户端IP:%s , 端口:%d . pid=%lu\n",
               inet_ntoa(clientDataPt->clientAddr.sin_addr),
               ntohs(clientDataPt->clientAddr.sin_port), GetCurrentThreadId());

        //关联socket和comPort，一旦关联的socket有IO完成，函数GetQueuedCompletionStatus就返回，并且会获取第三个参数(clientDataPt)
        CreateIoCompletionPort((HANDLE) clientDataPt->clientSocket, compPort, (ULONG_PTR) clientDataPt, 0);

        //分配用户空间缓冲区和overlapped
        IOData *ioDataPt = malloc(sizeof(IOData));
        memset(&ioDataPt->overlapped, 0, sizeof(OVERLAPPED));
        ioDataPt->wsabuf.len = BUF_SIZE;
        ioDataPt->wsabuf.buf = ioDataPt->buffer;
        ioDataPt->rwMode = READ;// IOCP本身不区分输入完成和输出完成状态,只通知完成IO状态
        //第6个参数,会在函数GetQueuedCompletionStatus时返回，因为结构体第一个元素的地址和结构体地址相同，因此相当于传递了ioDataPt
        WSARecv(
                clientDataPt->clientSocket,
                &ioDataPt->wsabuf,
                1, &ioDataPt->receiveBytes,
                &flags,
                &ioDataPt->overlapped,
                NULL
        );

    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

unsigned int WINAPI
echoThead(LPVOID
completionPortIO) {
HANDLE comPort = completionPortIO;
DWORD transBytes;
ClientData *clientDataPt;//第70行
IOData *ioDataPt;//第79行
SOCKET clientSocket;
DWORD flags = 0;
while (1) {
//阻塞等待事件发生
GetQueuedCompletionStatus(comPort,
&transBytes, (PULONG_PTR) &clientDataPt, (LPOVERLAPPED *) &ioDataPt,
INFINITE);
clientSocket = clientDataPt->clientSocket;
if (ioDataPt->rwMode == READ) { //读取数据完成事件
printf("[读]事件完成,transBytes=%lu\n", transBytes);
if (transBytes == 0) {//收到EOF，也就是FIN报文
printf("关闭连接事件,客户端IP:%s , 端口:%d\n",
inet_ntoa(clientDataPt
->clientAddr.sin_addr),
ntohs(clientDataPt
->clientAddr.sin_port));
closesocket(clientSocket);
free(clientDataPt);
free(ioDataPt);
continue;
}
memset(&ioDataPt->overlapped, 0, sizeof(OVERLAPPED));
ioDataPt->wsabuf.
len = transBytes;
ioDataPt->
rwMode = WRITE;
//这个WSASend也是一个异步写
WSASend(clientSocket,
&ioDataPt->wsabuf, 1, NULL, 0, &ioDataPt->overlapped, NULL);

//再次发送消息后接收到客户端消息
ioDataPt = malloc(sizeof(IOData));
memset(&ioDataPt->overlapped, 0, sizeof(OVERLAPPED));
ioDataPt->wsabuf.
len = BUF_SIZE;
ioDataPt->wsabuf.
buf = ioDataPt->buffer;
ioDataPt->
rwMode = READ;// IOCP本身不区分输入完成和输出完成状态,只通知完成IO状态
WSARecv(clientDataPt
->clientSocket, &ioDataPt->wsabuf, 1, &ioDataPt->receiveBytes, &flags,
&ioDataPt->overlapped,
NULL);
} else {
printf("[写]事件完成,transBytes=%lu\n", transBytes);
free(ioDataPt);
}
}

return 0;
}


#pragma clang diagnostic pop