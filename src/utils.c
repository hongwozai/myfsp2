/********************************************************************
 ** Copyright(c) 2016, luzeya
 ** All rights reserved
 **
 ** 文件名：utils.c
 ** 创建人：路泽亚
 ** 创建日期： 2016-08-23
 **
 ** 描  述：简要描述文件内容
 ** 注  意：本程序是否有些特殊说明
 ********************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

ssize_t readn(int sockfd, void *buf, size_t count)
{
    size_t nleft = count;
    char *tmp = (char *)buf;
    while (nleft > 0) {
        ssize_t nread = read(sockfd, tmp, nleft);
        if (nread == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        } else if (nread == 0) {
            /* 对方关闭 */
            return count - nleft;
        } else {
            nleft -= nread;
            tmp   += nread;
        }
    }
    return count;
}

ssize_t writen(int sockfd, void *buf, size_t count)
{
    size_t nleft = count;
    char *tmp = (char *)buf;
    while (nleft > 0) {
        ssize_t nwrite = write(sockfd, tmp, nleft);
        if (nwrite == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        } else if (nwrite == 0) {
            /* 对方关闭 */
            return count - nleft;
        } else {
            nleft -= nwrite;
            tmp   += nwrite;
        }
    }
    return count;
}

ssize_t readline(int sockfd, void *buf, size_t maxline)
{
    int i;
    int ret, nread;
    char *bufp = (char *)buf;
    int nleft = maxline;
    while (nleft > 0) {
        ret = recv(sockfd, bufp, nleft, MSG_PEEK);
        if (ret < 0) {
            return -1;
        } else if (ret == 0) {
            return 0;
        }
        /* 看看有无\n，然后再做决定 */
        nread = ret;
        for (i = 0; i < ret; i++) {
            if (bufp[i] == '\n') {
                /* 有\n, 直接返回 */
                ret = readn(sockfd, bufp, i+1);
                /* 因为已经在缓冲区内了，所以ret不等于i+1就一定错误 */
                if (ret != i+1) {
                    return -1;
                }
                /* 读到一行, 返回 */
                return ret;
            }
        }
        /* 无\n, 将buf中的全部读完后，继续再读(循环) */
        ret = readn(sockfd, bufp, nread);
        if (ret != nread) {
            return -1;
        }
        nleft -= nread;
        bufp += nread;
    }
    return ret;
}

static inline
void print_addr(struct sockaddr_in *addr)
{
    printf("ip: %s:%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
}

int print_sockaddr(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        printf("[%s, %d] getsocketname failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    print_addr(&addr);
    return 0;
}

int print_peeraddr(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getpeername(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        printf("[%s, %d] getpeername failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    print_addr(&addr);
    return 0;
}

int getlocalsockaddr(int sockfd, char *ip, int *port)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        printf("[%s, %d] getsockname failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);
    return 0;
}

int tcp_client()
{
    int sockfd;
    int optval;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("[%s, %d] socket failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) == -1){
        printf("[%s, %d] setsockopt failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int tcp_server(const char *ip, short port, int blacklog)
{
	int sockfd, optval;
	struct sockaddr_in sockaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
        printf("[%s, %d] socket failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
		return -1;
	}
	optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) == -1){
        printf("[%s, %d] setsockopt failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
        close(sockfd);
		return -1;
	}
	sockaddr.sin_family       =  AF_INET;
	sockaddr.sin_addr.s_addr  =  inet_addr(ip==NULL?"127.0.0.1":ip);
	sockaddr.sin_port         =  htons(port);
    if (port != 0) {
        if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1) {
            printf("[%s, %d] bind failure: %s\n",
                   __FILE__, __LINE__, strerror(errno));
            close(sockfd);
            return -1;
        }
    }
    if (listen(sockfd, blacklog?blacklog:SOMAXCONN) == -1) {
        printf("[%s, %d] listen failure: %s\n",
               __FILE__, __LINE__, strerror(errno));
        close(sockfd);
        return -1;
    }
    return sockfd;
}