#ifndef FTPD_CONTROLLER_H
#define FTPD_CONTROLLER_H

#include "session.h"
#include "parseconfig.h"
class FtpdController
{
public:
	FtpdController();
	~FtpdController();
	void loop();
private:
	void ftpd_init();	//初始化，包括创建套接字，加载配置文件

	 ParseConfig *parseconfig;
	 int listenfd;//监听套接字 
	 int connfd;//已连接套接字
	 pid_t pid;//进程pid
	 char* char_ip;//用于保存字符串ip
	 session_t sess;	//session结构体
};

#endif // FTPD_CONTROLLER_H
