/********************************************************************
 ** Copyright(c) 2016, luzeya
 ** All rights reserved
 **
 ** 文件名：utils.h
 ** 创建人：路泽亚
 ** 创建日期： 2016-08-23
 **
 ** 描  述：简要描述文件内容
 ** 注  意：本程序是否有些特殊说明
 ********************************************************************/
#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

ssize_t readn(int sockfd, void *buf, size_t count);
ssize_t writen(int sockfd, void *buf, size_t count);
ssize_t readline(int sockfd, void *buf, size_t maxline);

int print_sockaddr(int sockfd);
int print_peeraddr(int sockfd);
int getlocalsockaddr(int sockfd, char *ip, int *port);

int tcp_client();
int tcp_server(const char *ip, short port, int blacklog);

#endif /* UTILS_H */
