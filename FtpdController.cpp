#include "FtpdController.h"
#include "sysutil.h"
FtpdController::FtpdController():parseconfig(ParseConfig::getInstance())
{
	ftpd_init();
}

FtpdController::~FtpdController()
{//释放套接字资源
}

void FtpdController::ftpd_init()
{
	if (getuid() != 0)//如果不是root用户启动，则会退出
	{
		FTPD_LOG(ERROR,"the ftpd must be started as root");
	}
	//加载配置文件
	parseconfig->loadfile();
	//初始化会话结构体
	sess =
	{ 
		//控制连接
        0,-1,{0},{0},{0},{0},
        //数据连接
        NULL,-1,-1,
        //父子进程通道
   		-1,-1,
        //FTP协议状态	
        -1,0,NULL
	};
}

void FtpdController::loop()
{
	//创建一个tcp的服务器
	listenfd = sysutil::tcp_server(NULL,21);
	FTPD_LOG(DEBUG,"tcp_server 's listenfd:%d",listenfd);
	struct sockaddr_in addr;//用来保存对等方的地址
	while(1)
	{
		//建立连接
	    if ((connfd =  sysutil::accept_timeout(listenfd,&addr,0)) == -1)
	    {//失败
	    	FTPD_LOG(ERROR,"accept_timeout");
	    }
	    //转换成点分十进制的字符串，方便调试
	    char_ip = inet_ntoa(addr.sin_addr);
	    FTPD_LOG(DEBUG,"%s is connecting",char_ip);
	    pid = fork();//创建进程
	    if (-1 == pid)
	    {//fork失败
	    	FTPD_LOG(ERROR,"fork error");
	    }
	    else if (0 == pid)
	    {//子进程
	    	close(listenfd);//子进程无需处理监听
	    	//登记已连接的套接字
	    	sess.ctrl_fd = connfd;
	    	FTPD_LOG(DEBUG,"ctrl_fd:%d",sess.ctrl_fd);
	    	strcpy(sess.ip,char_ip);//拷贝ip
	    	FTPD_LOG(DEBUG,"子进程");
	    	//避免僵尸进程
			signal(SIGCHLD,SIG_IGN);
	    	//子进程开启一个新的会话
	    	begin_session(&sess);
	    }
	    else
	    {//父进程
	    	close(connfd);//父进程不需要处理连接，关闭已连接套接字继续等待其他客户端的连接
	    	FTPD_LOG(DEBUG,"父进程");
	    }
	}
}
