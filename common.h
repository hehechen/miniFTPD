/**
  *@brief：主要的库头文件及一些声明
  */ 

#ifndef _COMMON_H
#define _COMMON_H

#include <iostream>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>//套接字编程
#include <netinet/in.h>//地址
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <map>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
//不是#include <linux/capability.h>。。不知道别人是怎么通过编译的
#include <sys/capability.h>
#include <sys/syscall.h>    
#include <signal.h>
#include <sys/sendfile.h>
#include <sys/timerfd.h>  
#include <pthread.h>
#include <sys/epoll.h>

/**
 * 一个连接两个进程，将这两个进程抽象为一个会话
 * 会话结构体的定义如下
 */
#define MAX_COMMAND_LINE 1024   
#define MAX_COMMAND 32 
#define MAX_ARG 1024 
struct session_t
{
    //控制连接
    uid_t uid;//用户id
    int ctrl_fd;//已连接套接字
    char ip[16];//ip
    char cmdline[MAX_COMMAND_LINE];//命令行
    char cmd[MAX_COMMAND];//命令
    char arg[MAX_ARG];//参数
    //数据连接
    struct sockaddr_in* port_addr;//到时要连接的地址
    int pasv_listen_fd;//被动模式套接字
    int data_fd;//数据套接字
    
    //父子进程通道
    int parent_fd;
    int child_fd;    

    //FTP协议状态
    int is_ascii;//是否是ascii码状态
    long long restart_pos;//用于后期的断点续传
    char* rnfr_name;//重命名时用于保存文件名
};



//'\'后面不要加注释
/**
 *FTPD_LOG - 日志宏
 *输出日期，时间，日志级别，源码文件，行号，信息
 */
 //定义一个日志宏
#define DEBUG 0
#define INFO  1
#define WARN  2
#define ERROR 3
#define CRIT  4 

static const char* LOG_STR[] = { 
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "CRIT"
};

#define FTPD_LOG(level, format, ...) do{ \
    char msg[1024];                        \
    char buf[32];                                   \
    sprintf(msg, format, ##__VA_ARGS__);             \
    if (level >= 0) {\
        time_t now = time(NULL);                      \
        strftime(buf, sizeof(buf), "%Y%m%d %H:%M:%S", localtime(&now)); \
        fprintf(stdout, "[%s] [%s] [file:%s] [line:%d] %s\n",buf,LOG_STR[level],__FILE__,__LINE__, msg); \
        fflush (stdout); \
    }\
     if (level >= ERROR) {\
        perror(msg);    \
        exit(-1); \
    } \
} while(0)


//出错退出，为了和另一模块的代码兼容
#define ERR_EXIT(m) do { \
    FTPD_LOG(ERROR,m);\
} while (0);


#endif