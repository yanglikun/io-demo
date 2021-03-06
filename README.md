# io概念理解
## 某些功能只在特定平台提供比如IOCP，在linux不能运行
- 阻塞：blockServer.c (mac、linux)
- 非阻塞：nonblockServer.c(mac、linux)
- IO多路复用:multiplexing
  - select:selectServer.c (mac、linux)
  - poll:pollServer.c (mac、linux)
  - epoll:epollServer.c (linux)
  - IOCP(异步IO):IOCPServer.c (windows)
  - 水平触发、边缘触发:epollETLT.c (linux)
-  linux aio: 
   - 磁盘aio: AioReadMain.c (mac、linux)
   - socketaio: AioReadSocket.c (mac、linux)
