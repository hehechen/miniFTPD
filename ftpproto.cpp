#include "ftpproto.h"
#include "privpar.h"
#include "ftpcodes.h"
#include "common.h"
#include "sysutil.h"
#include "str_tool.h"
#include "ftpdIPC.h"
#include "TimerHeap.h"

//处理ftp命令
static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_cwd(session_t *sess);
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);
//传输参数命令
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_type(session_t *sess);
//服务命令
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_appe(session_t *sess);
static void do_list(session_t *sess);
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_mkd(session_t *sess);
static void do_rmd(session_t *sess);
static void do_dele(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_syst(session_t *sess);
static void do_feat(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);


static void do_site_chmod(session_t *sess, char *chmod_arg);
static void do_site_umask(session_t *sess, char *umask_arg);

map<string,function<void (session_t *)> > cmd_handler = {
	/* 访问控制命令 */
	{"USER",	do_user	},
	{"PASS",	do_pass	},
	{"CWD",		do_cwd	},
	{"XCWD",	do_cwd	},
	{"CDUP",	do_cdup	},
	{"XCUP",	do_cdup	},
	{"QUIT",	do_quit	},
	{"ACCT",	NULL	},
	{"SMNT",	NULL	},
	{"REIN",	NULL	},
	/* 传输参数命令 */
	{"PORT",	do_port	},
	{"PASV",	do_pasv	},
	{"TYPE",	do_type	},
	{"STRU",	/*do_stru*/NULL	},
	{"MODE",	/*do_mode*/NULL	},

	/* 服务命令 */
	{"RETR",	do_retr	},
	{"STOR",	do_stor	},
	{"APPE",	do_appe	},
	{"LIST",	do_list	},
	{"NLST",	do_nlst	},
	{"REST",	do_rest	},
	{"ABOR",	do_abor	},
	{"\377\364\377\362ABOR", do_abor},
	{"PWD",		do_pwd	},
	{"XPWD",	do_pwd	},
	{"MKD",		do_mkd	},
	{"XMKD",	do_mkd	},
	{"RMD",		do_rmd	},
	{"XRMD",	do_rmd	},
	{"DELE",	do_dele	},
	{"RNFR",	do_rnfr	},
	{"RNTO",	do_rnto	},
	{"SITE",	do_site	},
	{"SYST",	do_syst	},
	{"FEAT",	do_feat },
	{"SIZE",	do_size	},
	{"STAT",	do_stat	},
	{"NOOP",	do_noop	},
	{"HELP",	do_help	},
	{"STOU",	NULL	},
	{"ALLO",	NULL	}
};
/**
 * 子进程处理ftp请求
 * @param sess 指向FtpdController的一个session_t成员
 */
void handle_child(session_t *sess)
{
	ftp_reply(sess, FTP_GREET, "(miniftpd 0.1)");
	int ret;
	TimerHeap timer;	
	auto timer_handler = []	{ exit(EXIT_SUCCESS);	};	//空闲断开
	while (1)
	{
		memset(sess->cmdline, 0, sizeof(sess->cmdline));
		memset(sess->cmd, 0, sizeof(sess->cmd));
		memset(sess->arg, 0, sizeof(sess->arg));

		int id = timer.addTimer(10000000,timer_handler);		//10s没命令就断开
		ret = sysutil::readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE);
		if (ret == -1)
			FTPD_LOG(ERROR,"readline error");
		else if (ret == 0)	//说明客户端断开连接
			FTPD_LOG(ERROR,"client %s disconnect",sess->ip);

		FTPD_LOG(DEBUG,"cmdline=[%s]\n", sess->cmdline);
		FTPD_LOG(DEBUG,"CANCLE ID");
		timer.cancle(id);
		// 去除\r\n
		str_trim_crlf(sess->cmdline);
		printf("cmdline=[%s]\n", sess->cmdline);
		// 解析FTP命令与参数，空格前是命令，空格后是参数
		str_split(sess->cmdline, sess->cmd, sess->arg, ' ');		
		// 将命令转换为大写
		str_upper(sess->cmd);

		FTPD_LOG(DEBUG,"cmd=[%s] arg=[%s]\n", sess->cmd, sess->arg);
		//调用相应的处理函数
		auto it = cmd_handler.find(sess->cmd);
		if(it != cmd_handler.end())
			(it->second)(sess);
		else
			ftp_reply(sess,FTP_BADCMD,"Unknown command.");
	}
}
/**
 * ftp应答
 * @param sess   [description]
 * @param status 状态在ftpcodes.h文件中定义
 * @param text   [description]
 */
void ftp_reply(session_t *sess, int status, const char *text)
{
	char buf[1024] = {0};
	sprintf(buf, "%d %s\r\n", status, text);
	sysutil::writen(sess->ctrl_fd, buf, strlen(buf));
}

void ftp_lreply(session_t *sess, int status, const char *text)
{
	char buf[1024] = {0};
	sprintf(buf, "%d-%s\r\n", status, text);
	sysutil::writen(sess->ctrl_fd, buf, strlen(buf));
}

//处理ftp命令
static void do_user(session_t *sess)
{
	//USER chen
	struct passwd *pw = getpwnam(sess->arg);
	if (pw == NULL)
	{
		// 用户不存在
		ftp_reply(sess, FTP_LOGINERR, "Fuck you!user incorrect.");
		return;
	}

	sess->uid = pw->pw_uid;
	ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");
}
static void do_pass(session_t *sess)
{
	// PASS 123456
	struct passwd *pw = getpwuid(sess->uid);
	if (pw == NULL)
	{
		// 用户不存在
		ftp_reply(sess, FTP_LOGINERR, "Fuck you!user don't exit.");
		return;
	}

	
	struct spwd *sp = getspnam(pw->pw_name);//获取影子文件信息
	if (sp == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}

	// 影子文件中的密码是已经加密过的，故要将明文进行加密方便比较
	char *encrypted_pass = crypt(sess->arg, sp->sp_pwdp);
	FTPD_LOG(DEBUG,"encrypted_pass=[%s]\n", encrypted_pass);
	// 验证密码
	if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)
	{
		ftp_reply(sess, FTP_LOGINERR, "Fuck you!password incorrect.");
		return;
	}

	// signal(SIGURG, handle_sigurg);
	// activate_sigurg(sess->ctrl_fd);

	umask(077);		//一般用户的权限是077
	setegid(pw->pw_gid);
	seteuid(pw->pw_uid);
	chdir(pw->pw_dir);	//改变当前目录
	ftp_reply(sess, FTP_LOGINOK, "Login successful.");
}
/**
 * 改变当前路径
 * @param sess [description]
 */
static void do_cwd(session_t *sess)
{
	if (chdir(sess->arg) < 0)
	{
		ftp_reply(sess, FTP_FILEFAIL, "Failed to change directory.");
		return;
	}

	ftp_reply(sess, FTP_CWDOK, "Directory successfully changed.");
}
/**
 * 返回上层路径
 * @param sess [description]
 */
static void do_cdup(session_t *sess)
{
	if (chdir("..") < 0)
	{
		ftp_reply(sess, FTP_FILEFAIL, "Failed to change directory.");
		return;
	}

	ftp_reply(sess, FTP_CWDOK, "Directory successfully changed.");
}
static void do_quit(session_t *sess)
{
	ftp_reply(sess, FTP_GOODBYE, "Goodbye."); 
	exit(EXIT_SUCCESS); 
}
/**
 * 主动模式创建数据连接
 * @param sess [description]
 */
static void do_port(session_t *sess)
{
	//PORT 192,168,0,100,123,233
	unsigned int v[6];
	//从sess->arg中获取数据
	sscanf(sess->arg, "%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);
	sess->port_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memset(sess->port_addr, 0, sizeof(struct sockaddr_in));
	sess->port_addr->sin_family = AF_INET;
	unsigned char *p = (unsigned char *)&sess->port_addr->sin_port;
	p[0] = v[0];//保存端口
	p[1] = v[1];

	p = (unsigned char *)&sess->port_addr->sin_addr;
	p[0] = v[2];//保存ip
	p[1] = v[3];
	p[2] = v[4];
	p[3] = v[5];

	ftp_reply(sess, FTP_PORTOK, "PORT command successful. Consider using PASV.");
}
/**
 * 被动模式创建数据连接
 * @param sess [description]
 */
static void do_pasv(session_t *sess)
{
	//Entering Passive Mode (192,168,244,100,101,46).

	// char ip[16] = {0};
	// sysutil::getlocalip(ip);

	// priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_LISTEN);
	// unsigned short port = (int)priv_sock_get_int(sess->child_fd);

	// unsigned int v[4];
	// sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
	// char text[1024] = {0};
	// sprintf(text, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).", 
	// 	v[0], v[1], v[2], v[3], port>>8, port&0xFF);

	// ftp_reply(sess, FTP_PASVOK, text);
	// 	//被动模式，创建套接字，绑定端口，监听
	char ip[16] = {0};
	sysutil::getlocalip(ip);//获取本地IP

	sess->pasv_listen_fd = sysutil::tcp_server(ip,0);//0表示端口任意
	FTPD_LOG(DEBUG,"pasv_listen_fd:%d",sess->pasv_listen_fd);
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if ((getsockname(sess->pasv_listen_fd,(struct sockaddr*)&addr,&addrlen)) < 0)//获取本地的地址信息，套接字必须已知
	{	
		 FTPD_LOG(DEBUG,"getsockname");
	}
	//这里端口任意的话，可能被防火墙关了
	unsigned short port = ntohs(addr.sin_port);//保存端口信息，网络字节序转为主机字节序
	unsigned int v[4];
	sscanf(ip,"%u.%u.%u.%u",&v[0],&v[1],&v[2],&v[3]);
	char text[1024] = {0};
	sprintf(text,"Entering Passive Mode (%u,%u,%u,%u,%u,%u).",v[0],v[1],v[2],v[3],port>>8,port&0xFF);//port是两个字节，这里分别获取高八位和低八位
	ftp_reply(sess,FTP_PASVOK,text);
}
/**
 * TYPE A:转换到ASCII码模式
 * TYPE I:转化到二进制模式
 * @param sess [description]
 */
static void do_type(session_t *sess)
{
	if (strcmp(sess->arg, "A") == 0)
	{
		sess->is_ascii = 1;
		ftp_reply(sess, FTP_TYPEOK, "Switching to ASCII mode.");
	}
	else if (strcmp(sess->arg, "I") == 0)
	{
		sess->is_ascii = 0;
		ftp_reply(sess, FTP_TYPEOK, "Switching to Binary mode.");
	}
	else
	{
		ftp_reply(sess, FTP_BADCMD, "Unrecognised TYPE command.");
	}
}
//服务命令
/**
 * 下载文件，断点续传
 * @param sess [description]
 */
static void do_retr(session_t *sess)
{
	//创建数据连接,主动用connect，被动用accept
	if (get_transfer_fd(sess))//连接失败
	{//get_transfer_fd里面会开启数据通道的传输闹钟，等下传输结束记得关掉
		 FTPD_LOG(DEBUG,"get_transfer_fd(sess)");
		 return;
	}
	 FTPD_LOG(DEBUG,"data_fd int do_retr:%d",sess->data_fd);
	//保存断点
	long long offset = sess->restart_pos;
	sess->restart_pos = 0;//断点位置为0

	int fd = open(sess->arg,O_RDONLY);//以只读方式打开文件
	if (-1 == fd)
	{
		//打开失败
		ftp_reply(sess,FTP_FILEFAIL,"Failed to open file.");
		return;
	}
	int ret;
	//加读锁
	ret = sysutil::lock_file_read(fd);
	if (-1 ==ret)
	{//加锁失败
		ftp_reply(sess,FTP_FILEFAIL,"Failed to open file.");
	}
	//判断是否是普通文件，设备文件是不能下载的
	struct stat sbuf;
	ret = fstat(fd,&sbuf);//将文件状态保存在sbuf中
	if (!S_ISREG(sbuf.st_mode))
	{//不是一个普通文件
		ftp_reply(sess,FTP_FILEFAIL,"Failed to open file.");
		return;
	}
	
	if (offset != 0)//如果有断点
	{
		ret = lseek(fd,offset,SEEK_SET);//定位断点
		if (-1 == ret)
		{
			//定位失败
			ftp_reply(sess,FTP_FILEFAIL,"Failed to lseek");
			return;
		}
	}
	char text[1024] = {0};
	if (sess->is_ascii)//TYPE A
	{
		 //ASCII码模式(实际上我们这里都是以二进制方式进行传输)
		sprintf(text,"Opening ASCII mode data connection for %s (%lld bytes).",
			 sess->arg,(long long)sbuf.st_size);
	}
	else//TYPE I
	{
		//二进制模式
		sprintf(text,"Opening BINARY mode data connection for %s (%lld bytes).",
			 sess->arg,(long long)sbuf.st_size);
	}
	//150响应
	ftp_reply(sess,FTP_DATACONN,text);
	//下载文件
	int flag = 0;//标志变量
	long long bytes_to_send = sbuf.st_size;//文件大小
	if (offset > bytes_to_send)//断点位置不对
	{
		bytes_to_send = 0;
	}
	else
	{
		bytes_to_send -= offset;//只传输断点到文件结束的大小
	}
	//开始传输
	while(bytes_to_send)
	{//sendfile直接在内核空间操作，不涉及拷贝，效率较高
		int num_this_time = bytes_to_send > 4096 ? 4096:bytes_to_send;
		ret = sendfile(sess->data_fd,fd,NULL,num_this_time);//不会返回EINTR,ret是发送成功的字节数
		if (-1 == ret)
		{
			//发送失败
			flag = 2;
			break;
		}
		bytes_to_send -= ret;//更新剩下的字节数
	}

	if (bytes_to_send == 0)
	{
		//发送成功
		flag = 0;
	}
	//关闭数据链接套接字,客户端是通过判断套接字关闭从而判断数据是否接收完毕
	close(sess->data_fd);
	sess->data_fd = -1;
	//关闭文件
    close(fd);
	if (0 == flag)//成功并且没有收到abor
	{
		//226响应
		ftp_reply(sess,FTP_TRANSFEROK,"Transfer complete.");
	}
	else if (1 == flag)
	{//文件读取失败 451
		 ftp_reply(sess,FTP_BADSENDFILE,"Failure reading from local file.");
	}
	else if (2 == flag)
	{//文件发送失败 426
		ftp_reply(sess,FTP_BADSENDNET,"Failure writting to network stream.");
	}
}
/**
 * 以stor方式上传文件
 * @param sess [description]
 */
static void do_stor(session_t *sess)
{
	upload_common(sess,0);//0表示STOR方式
}
/**
 * 以appe方式上传文件
 * @param sess [description]
 */
static void do_appe(session_t *sess)
{
	upload_common(sess,1);//1表示APPE方式
}


/**
 * 创建数据连接，传输列表，关闭套接字
 * @param sess [description]
 */
static void do_list(session_t *sess)
{
	// 创建数据连接
	if (get_transfer_fd(sess) == -1)
	{
		FTPD_LOG(DEBUG,"send list error");
		return;
	}
	// 150
	ftp_reply(sess, FTP_DATACONN, "Here comes the directory listing.");

	// 传输列表
	list_common(sess, 1);
	// 关闭数据套接字
	close(sess->data_fd);
	sess->data_fd = -1;
	// 226
	ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK.");
}

/**
 * 创建数据连接
 * @param  sess [description]
 * @return      成功返回0，失败返回-1
 */
int get_transfer_fd(session_t *sess)
{
	FTPD_LOG(DEBUG,"pasv_listen_fd:%d",sess->pasv_listen_fd);
	// 检测是否收到PORT或者PASV命令
	if (!port_active(sess) && !pasv_active(sess))
	{
		ftp_reply(sess, FTP_BADSENDCONN, "Use PORT or PASV first.");
		return -1;
	}
	int ret = -1;
	// 如果是主动模式
	if (port_active(sess))
	{
		if (get_port_fd(sess) == 0)
		{
			ret = 0;
		}
	}
	if (pasv_active(sess))
	{
		if (get_pasv_fd(sess) == 0)
		{
			ret = 0;
		}
	}
	//将主动连接的地址清空，调用方负责关闭数据连接
	if (sess->port_addr)
	{
		free(sess->port_addr);
		sess->port_addr = NULL;
	}

	return ret;

}

/**
 * 检测主动模式是否被激活
 * @param  sess [description]
 * @return      是 返回1
 *              否  返回0
 */
int port_active(session_t *sess)
{
	if (sess->port_addr)
	{
		if (sess->pasv_listen_fd != -1)
		{
			FTPD_LOG(ERROR,"both port an pasv are active");
		}
		return 1;
	}

	return 0;
}
/**
 * 检查被动模式是否被激活，注意主动和被动不能被同时激活
 * @param  sess [description]
 * @return      是 返回1
 *              否 返回0
 */
int pasv_active(session_t *sess)
{
	if(sess->pasv_listen_fd != -1)
	{//sess结构体在初始化的时候将pasv_listen_fd设为-1，如果被改变说明已经激活被动模式
		if(port_active(sess))
			FTPD_LOG(ERROR,"both port an pasv are active");
		return 1;
	}
	return 0;
}

/**
 * 获取主动连接的数据套接字
 * @param  sess [description]
 * @return      成功返回0，失败返回-1
 */
int get_port_fd(session_t* sess)
{
/*	向nobody发送PRIV_SOCK_GET_DATA_SOCK命令        
	向nobody发送一个整数port		       
	向nobody发送一个字符串ip                      
*/
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
	unsigned short port = ntohs(sess->port_addr->sin_port);
	char *ip = inet_ntoa(sess->port_addr->sin_addr);
	priv_sock_send_int(sess->child_fd, (int)port);
	priv_sock_send_buf(sess->child_fd, ip, strlen(ip));

	char res = priv_sock_get_result(sess->child_fd);
	if (res == PRIV_SOCK_RESULT_BAD)
	{
		return -1;
	}
	else if (res == PRIV_SOCK_RESULT_OK)
	{
		sess->data_fd = priv_sock_recv_fd(sess->child_fd);
	}

	return 0;
}
/**
 * 获取被动连接的数据连接套接字
 * @param  sess [description]
 * @return      成功返回0，失败返回-1
 */
int get_pasv_fd(session_t *sess)
{
	// priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACCEPT);
	// char res = priv_sock_get_result(sess->child_fd);
	// if (res == PRIV_SOCK_RESULT_BAD)
	// {
	// 	return -1;
	// }
	// else if (res == PRIV_SOCK_RESULT_OK)
	// {
	// 	sess->data_fd = priv_sock_recv_fd(sess->child_fd);
	// }

	// return 0;
	//do_pasv中定义了一个tcp服务器，完成绑定和监听，这里完成accept
	int fd = sysutil::accept_timeout(sess->pasv_listen_fd,NULL,0);
	close(sess->pasv_listen_fd);//监听套接字此时已经没用，关掉
	if (-1 == fd)
	{
		return -1;
	}
	sess->data_fd = fd;//数据连接套接字
	return 0;
}


static void do_nlst(session_t *sess)
{

}
/**
 * 断点续传
 * @param sess [description]
 */
static void do_rest(session_t *sess)
{
	sess->restart_pos = str_to_longlong(sess->arg);
	char text[1024] = {0};
	sprintf(text, "Restart position accepted (%lld).", sess->restart_pos);
	ftp_reply(sess, FTP_RESTOK, text);
}
static void do_abor(session_t *sess)
{

}
/**
 * 回复当前路径，要加上双引号
 * @param sess [description]
 */
static void do_pwd(session_t *sess)
{
	char text[1024] = {0};
	char dir[1024+1] = {0};
	getcwd(dir, 1024);		//获取当前工作目录
	sprintf(text, "\"%s\"", dir);

	ftp_reply(sess, FTP_PWDOK, text);
}
/**
 * 创建目录
 * @param sess [description]
 */
static void do_mkd(session_t *sess)
{
	// 0777 & umask
	if (mkdir(sess->arg, 0777) < 0)
	{
		ftp_reply(sess, FTP_FILEFAIL, "Create directory operation failed.");
		return;
	}
	
	char text[4096] = {0};
	if (sess->arg[0] == '/')
	{
		sprintf(text, "%s created", sess->arg);
	}
	else
	{
		char dir[4096+1] = {0};
		getcwd(dir, 4096);	//获取当前路径
		if (dir[strlen(dir)-1] == '/')
		{
			sprintf(text, "%s%s created", dir, sess->arg);
		}
		else
		{
			sprintf(text, "%s/%s created", dir, sess->arg);
		}
	}

	ftp_reply(sess, FTP_MKDIROK, text);
}
/**
 * 删除空文件夹
 * @param sess [description]
 */
static void do_rmd(session_t *sess)
{
	if (rmdir(sess->arg) < 0)
	{
		ftp_reply(sess, FTP_FILEFAIL, "Remove directory operation failed.");
	}

	ftp_reply(sess, FTP_RMDIROK, "Remove directory operation successful.");
}
/**
 * 删除文件
 * @param sess [description]
 */
static void do_dele(session_t *sess)
{
	if (unlink(sess->arg) < 0)
	{
		ftp_reply(sess, FTP_FILEFAIL, "Delete operation failed.");
		return;
	}

	ftp_reply(sess, FTP_DELEOK, "Delete operation successful.");
}
/**
 * 文件重命名，先发送要目标文件的文件名
 * @param sess [description]
 */
static void do_rnfr(session_t *sess)
{//保存目标文件名
	sess->rnfr_name = (char *)malloc(strlen(sess->arg) + 1);
	memset(sess->rnfr_name, 0, strlen(sess->arg) + 1);
	strcpy(sess->rnfr_name, sess->arg);
	ftp_reply(sess, FTP_RNFROK, "Ready for RNTO.");
}
/**
 * 文件重命名，在rnfr后发送
 * @param sess [description]
 */
static void do_rnto(session_t *sess)
{
	if (sess->rnfr_name == NULL)
	{//要先发送RNFR指定目标文件
		ftp_reply(sess, FTP_NEEDRNFR, "RNFR required first.");
		return;
	}
	//重命名
	rename(sess->rnfr_name, sess->arg);

	ftp_reply(sess, FTP_RENAMEOK, "Rename successful.");

	free(sess->rnfr_name);
	sess->rnfr_name = NULL;
}
static void do_site(session_t *sess)
{

}
/**
 * 回复系统信息
 * @param sess [description]
 */
static void do_syst(session_t *sess)
{
	ftp_reply(sess, FTP_SYSTOK, "UNIX Type: Ubuntu chen");

}
/**
 * 回复服务器的特性
 * @param sess 
 */
static void do_feat(session_t *sess)
{
	ftp_lreply(sess, FTP_FEAT, "Features:");
	sysutil::writen(sess->ctrl_fd, " EPRT\r\n", strlen(" EPRT\r\n"));
	sysutil::writen(sess->ctrl_fd, " EPSV\r\n", strlen(" EPSV\r\n"));
	sysutil::writen(sess->ctrl_fd, " MDTM\r\n", strlen(" MDTM\r\n"));
	sysutil::writen(sess->ctrl_fd, " PASV\r\n", strlen(" PASV\r\n"));
	sysutil::writen(sess->ctrl_fd, " REST STREAM\r\n", strlen(" REST STREAM\r\n"));
	sysutil::writen(sess->ctrl_fd, " SIZE\r\n", strlen(" SIZE\r\n"));
	sysutil::writen(sess->ctrl_fd, " TVFS\r\n", strlen(" TVFS\r\n"));
	sysutil::writen(sess->ctrl_fd, " UTF8\r\n", strlen(" UTF8\r\n"));
	ftp_reply(sess, FTP_FEAT, "End");
}
/**
 * 获取文件大小
 * @param sess [description]
 */
static void do_size(session_t *sess)
{
	//550 Could not get file size.

	struct stat buf;
	if (stat(sess->arg, &buf) < 0)
	{
		ftp_reply(sess, FTP_FILEFAIL, "SIZE operation failed.");
		return;
	}

	if (!S_ISREG(buf.st_mode))
	{//不是普通的文件
		ftp_reply(sess, FTP_FILEFAIL, "Could not get file size.");
		return;
	}

	char text[1024] = {0};
	sprintf(text, "%lld", (long long)buf.st_size);
	ftp_reply(sess, FTP_SIZEOK, text);
}
static void do_stat(session_t *sess)
{

}
/**
 * keeplive
 * @param sess [description]
 */
static void do_noop(session_t *sess)
{
	ftp_reply(sess,FTP_NOOPOK,"NOOP ok.");
}
static void do_help(session_t *sess)
{

}

/**
 * 列出目录
 * @param  sess   [description]
 * @param  detail [description]
 * @return        成功返回0，失败返回-1
 */
int list_common(session_t *sess, int detail)
{
	DIR *dir = opendir(".");	//当前路径
	if (dir == NULL)
	{
		return -1;
	}

	struct dirent *dt;
	struct stat sbuf;
	
	 	 // struct dirent {
         //       ino_t          d_ino;       /* inode number */
         //       off_t          d_off;       /* not an offset; see NOTES */
         //       unsigned short d_reclen;     length of this record 
         //       unsigned char  d_type;      /* type of file; not supported
         //                                      by all filesystem types */
         //       char           d_name[256]; /* filename */
         //   };

	 
	while ((dt = readdir(dir)) != NULL)
	{
		if (lstat(dt->d_name, &sbuf) < 0)//lstat和stat的区别在于lstat返回符号链接文件本身的信息
		{
			continue;
		}
		if (dt->d_name[0] == '.')//"."开头的是隐藏文件，不显示
			continue;

		char buf[1024] = {0};
		if (detail)
		{
			const char *perms = sysutil::statbuf_get_perms(&sbuf);		

			//按一定的格式获取权限信息 用户名 组名 大小 时间等信息			
			int off = 0;
			off += sprintf(buf, "%s ", perms);
			off += sprintf(buf + off, " %3d %-8d %-8d ", sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);
			off += sprintf(buf + off, "%8lu ", (unsigned long)sbuf.st_size);

			const char *datebuf = sysutil::statbuf_get_date(&sbuf);
			off += sprintf(buf + off, "%s ", datebuf);
			if (S_ISLNK(sbuf.st_mode))
			{
				char tmp[1024] = {0};
				readlink(dt->d_name, tmp, sizeof(tmp));
				off += sprintf(buf + off, "%s -> %s\r\n", dt->d_name, tmp);
			}
			else
			{
				off += sprintf(buf + off, "%s\r\n", dt->d_name);
			}
		}
		else
		{//简要信息只返回文件名
			sprintf(buf, "%s\r\n", dt->d_name);
		}
		
		sysutil::writen(sess->data_fd, buf, strlen(buf));
	}

	closedir(dir);

	return 1;
}

/**
 * 上传文件
 * @param sess      [description]
 * @param is_append 0表示以stor方式，1表示以appe方式
 */
void upload_common(session_t *sess, int is_append)
{
	// 创建数据连接
	if (get_transfer_fd(sess) == -1)
	{
		return;
	}

	long long offset = sess->restart_pos;
	sess->restart_pos = 0;

	// 打开文件
	int fd = open(sess->arg, O_CREAT | O_WRONLY, 0666);
	if (fd == -1)
	{
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
		return;
	}

	int ret;
	// 加写锁
	ret = sysutil::lock_file_write(fd);
	if (ret == -1)
	{
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
		return;
	}

	// STOR
	// REST+STOR
	// APPE
	if (!is_append && offset == 0)		// STOR
	{
		ftruncate(fd, 0);//把文件的长度清零
		if (lseek(fd, 0, SEEK_SET) < 0)
		{
			ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
			return;
		}
	}
	else if (!is_append && offset != 0)		// REST+STOR
	{
		if (lseek(fd, offset, SEEK_SET) < 0)
		{
			ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
			return;
		}
	}
	else if (is_append)				// APPE
	{//在文件尾部添加
		if (lseek(fd, 0, SEEK_END) < 0)
		{
			ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
			return;
		}
	}
	struct stat sbuf;
	ret = fstat(fd, &sbuf);
	if (!S_ISREG(sbuf.st_mode))
	{//不是普通文件
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
		return;
	}

	// 150
	char text[1024] = {0};
	if (sess->is_ascii)
	{
		sprintf(text, "Opening ASCII mode data connection for %s (%lld bytes).",
			sess->arg, (long long)sbuf.st_size);
	}
	else
	{
		sprintf(text, "Opening BINARY mode data connection for %s (%lld bytes).",
			sess->arg, (long long)sbuf.st_size);
	}

	ftp_reply(sess, FTP_DATACONN, text);

	int flag = 0;
	// 上传文件

	char buf[1024];

	while (1)
	{
		ret = read(sess->data_fd, buf, sizeof(buf));
		if (ret == -1)
		{
			if (errno == EINTR)
			{//被信号中断
				continue;
			}
			else
			{//读取失败
				flag = 2;
				break;
			}
		}
		else if (ret == 0)
		{//读取完毕
			flag = 0;
			break;
		}

	//	limit_rate(sess, ret, 1); //限速功能尚未实现
		// if (sess->abor_received)
		// {
		// 	flag = 2;
		// 	break;
		// }

		if (sysutil::writen(fd, buf, ret) != ret)
		{//写入文件失败
			flag = 1;
			break;
		}
	}


	// 关闭数据套接字，使客户端知道数据传输完毕
	close(sess->data_fd);
	sess->data_fd = -1;

	close(fd);

	if (flag == 0)
	{//成功
		// 226
		ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete.");
	}
	else if (flag == 1)
	{//文件写入本地失败
		// 451
		ftp_reply(sess, FTP_BADSENDFILE, "Failure writting to local file.");
	}
	else if (flag == 2)
	{//从网络读取数据失败
		// 426
		ftp_reply(sess, FTP_BADSENDNET, "Failure reading from network stream.");
	}
}
