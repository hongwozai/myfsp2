/********************************************************************
 ** Copyright(c) 2016, luzeya
 ** All rights reserved
 **
 ** 文件名：fsp.h
 ** 创建人：路泽亚
 ** 创建日期： 2016-08-23
 **
 ** 描  述：简要描述文件内容
 ** 注  意：本程序是否有些特殊说明
 ********************************************************************/
#ifndef FSP_H
#define FSP_H

int get_file(int sockfd, const char *path);
int put_file(int sockfd, const char *path);

#endif /* FSP_H */
