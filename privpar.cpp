#include "privpar.h"
#include "ftpdIPC.h"
#include "sysutil.h"


PrivPar::PrivPar():parseconfig(ParseConfig::getInstance())
{

}
/**
 * 本函数供父进程使用，将root进程设为nobody进程
 */
void minimize_privace()
{
	struct passwd* pw = getpwnam("nobody");
	if(pw == NULL)
		FTPD_LOG(ERROR,"getpw error");
	if(setegid(pw->pw_gid) == -1)
		FTPD_LOG(ERROR,"setegid error");
	if(seteuid(pw->pw_uid) == -1)
		FTPD_LOG(ERROR,"seteuid error");

	struct __user_cap_header_struct cap_header;
	struct __user_cap_data_struct cap_data;

	memset(&cap_header, 0, sizeof(cap_header));
	memset(&cap_data, 0, sizeof(cap_data));

	cap_header.version = _LINUX_CAPABILITY_VERSION_1;
	cap_header.pid = 0;

	__u32 cap_mask = 0;	//设置权限
	cap_mask |= (1 << CAP_NET_BIND_SERVICE);	//获得绑定端口的权限

	cap_data.effective = cap_data.permitted = cap_mask;
	cap_data.inheritable = 0;		//不能被继承

	capset(&cap_header, &cap_data);
}

void PrivPar::handle_parent(session_t *sess)
{
	minimize_privace();
	char cmd;
	while (1)
	{
		cmd = priv_sock_get_cmd(sess->parent_fd);
		// 处理内部命令
		switch (cmd)
		{
		case PRIV_SOCK_GET_DATA_SOCK:
			privop_pasv_get_data_sock(sess);
			break;
		case PRIV_SOCK_PASV_ACTIVE:
			privop_pasv_active(sess);
			break;
		case PRIV_SOCK_PASV_LISTEN:
			privop_pasv_listen(sess);
			break;
		case PRIV_SOCK_PASV_ACCEPT:
			privop_pasv_accept(sess);
			break;		
		}
	}
}


/**
 * 创建数据连接通道
 * @param sess [description]
 */
void PrivPar::privop_pasv_get_data_sock(session_t *sess)
{
	/*
	nobody进程接收PRIV_SOCK_GET_DATA_SOCK命令
进一步接收一个整数，也就是port
接收一个字符串，也就是ip

fd = socket 
bind(20)
connect(ip, port);

OK
send_fd
BAD
*/
	unsigned short port = (unsigned short)priv_sock_get_int(sess->parent_fd);
	char ip[16] = {0};
	priv_sock_recv_buf(sess->parent_fd, ip, sizeof(ip));
	//从ftp服务进程获取IP地址
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	int fd = sysutil::tcp_client(20);
	if (fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	if (sysutil::connect_timeout(fd, &addr, parseconfig->get_accept_timeout()) < 0)
	{
		close(fd);
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}

	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd, fd);
	close(fd);
}

void PrivPar::privop_pasv_active(session_t *sess)
{
	int active;
	if (sess->pasv_listen_fd != -1)
	{
		active = 1;
	}
	else
	{
		active = 0;
	}

	priv_sock_send_int(sess->parent_fd, active);
}

void PrivPar::privop_pasv_listen(session_t *sess)
{
	char ip[16] = {0};
	sysutil::getlocalip(ip);

	sess->pasv_listen_fd = sysutil::tcp_server(ip, 0);	
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if (getsockname(sess->pasv_listen_fd, (struct sockaddr *)&addr, &addrlen) < 0)
	{
		ERR_EXIT("getsockname");
	}

	unsigned short port = ntohs(addr.sin_port);

	priv_sock_send_int(sess->parent_fd, (int)port);
}

void PrivPar::privop_pasv_accept(session_t *sess)
{
	int fd = sysutil::accept_timeout(sess->pasv_listen_fd, NULL, parseconfig->get_accept_timeout());
	close(sess->pasv_listen_fd);
	sess->pasv_listen_fd = -1;

	if (fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}

	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd, fd);
	close(fd);
}
