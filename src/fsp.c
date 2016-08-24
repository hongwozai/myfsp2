/********************************************************************
 ** Copyright(c) 2016, luzeya
 ** All rights reserved
 **
 ** 文件名：fsp.c
 ** 创建人：路泽亚
 ** 创建日期： 2016-08-23
 **
 ** 描  述：协议相关实现
 ** 注  意：
 ********************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include "utils.h"

static inline int file_length(const char *path);

/**
 * 发送文件先传文件长度
 * path是路径
 * 返回-1失败，返回0成功
 */
int put_file(int sockfd, const char *path)
{
    int fd;
    int len = file_length(path);
    int netlen = htonl(len);

    if (sizeof(len) != writen(sockfd, &netlen, sizeof(netlen))) {
        /* 传长度失败 */
        return -1;
    }

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("[%s %d] Can't open file %s!(%s)\n",
               __FILE__, __LINE__, path, strerror(errno));
        return -1;
    }
    /* 零拷贝 */
    if (-1 == sendfile(sockfd, fd, NULL, len)) {
        printf("[%s %d] Can't transfer file %s!(%s)\n",
               __FILE__, __LINE__, path, strerror(errno));
        return -1;
    }
    close(fd);
    return 0;
}

/**
 * 接收文件也先传文件长度
 * path要存储的文件名
 * 返回-1失败，返回0成功，返回1链接关闭
 */
int get_file(int sockfd, const char *path)
{
    int len;
    int ret;
    int fd, pipefd[2];

    if (sizeof(len) != readn(sockfd, &len, sizeof(len))) {
        return -1;
    }
    len = ntohl(len);
    /* 零拷贝 */
    if (-1 == pipe(pipefd)) {
        printf("[%s %d] Can't create pipe!(%s)\n",
               __FILE__, __LINE__, strerror(errno));
        return -1;
    }

    fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd == -1) {
        printf("[%s %d] Can't create %s!(%s)\n",
               __FILE__, __LINE__, path, strerror(errno));
        return -1;
    }
    /* TODO: 此处还是可以使用异常，当没有数据接受时(链接关闭)异常直接跳出 */
    while (len > 0) {
        /* socket写进管道，再从管道拿出 */
        ret = splice(sockfd, NULL, pipefd[1], NULL, len,
                     SPLICE_F_MORE | SPLICE_F_MOVE);
        if (-1 == ret) {
            printf("[%s %d] splice failure!(%s)\n",
                   __FILE__, __LINE__, strerror(errno));
            return -1;
        }
        /* 链接关闭时 */
        if (0 == ret) {
            return 1;
        }
        ret = splice(pipefd[0], NULL, fd, NULL, ret,
                     SPLICE_F_MORE | SPLICE_F_MOVE);
        if (-1 == ret) {
            printf("[%s %d] splice failure!(%s)\n",
                   __FILE__, __LINE__, strerror(errno));
            return -1;
        }
        len -= ret;
    }
    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}

static inline
int file_length(const char *path)
{
    struct stat st;
    /* 内部使用，认为文件一定存在，不考虑错误情况 */
    stat(path, &st);
    return st.st_size;
}

/* 是目录返回0，不是目录返回1 */
int isdir(const char *path)
{
    struct stat st;

    /* 假定文件存在 */
    stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
        return 0;
    }
    return 1;
}

/* 文件存在返回0，文件不存在返回1，运行失败返回-1 */
int ensure_file_exist(const char *filename)
{
    DIR *dir;
    struct dirent *dirent;

    dir = opendir(".");
    if (!dir) {
        printf("[%s, %d]current dir can't open!(%s)\n",
               __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    while ((dirent = readdir(dir)) != NULL) {
        if (strcmp(filename, dirent->d_name) == 0) {
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return 1;
}


int rm_file(const char *filename)
{
    DIR *dir;
    char buf[1024];
    struct stat st;
    struct dirent *dirent;

    if (stat(filename, &st) == -1) {
        printf("[%s %d]filename error!(%s)\n",
               __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    if (S_ISREG(st.st_mode)) {
        if (unlink(filename) == -1) {
            printf("[%s %d]File can't delete!(%s)\n",
                   __FILE__, __LINE__, strerror(errno));
        }
    } else if (S_ISDIR(st.st_mode)) {
        dir = opendir(filename);
        if (dir == NULL) {
            printf("[%s %d]Can't open dir!(%s)\n",
                   __FILE__, __LINE__, strerror(errno));
            return -1;
        }
        errno = 0;
        for (dirent = readdir(dir);
             dirent != NULL;
             dirent = readdir(dir)) {
            if (strncmp(dirent->d_name, ".", 1) == 0) {
                continue;
            }
            sprintf(buf, "%s/%s", filename, dirent->d_name);
            if (rm_file(buf) == -1) {
                return -1;
            }
        }
        if (errno != 0) {
            printf("[%s %d]Read dir error!(%s)\n",
                   __FILE__, __LINE__, strerror(errno));
            return -1;
        }
        if (rmdir(filename) == -1) {
            printf("[%s %d]Can't remove dir!(%s)\n",
                   __FILE__, __LINE__, strerror(errno));
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}