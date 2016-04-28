#include "ftpdIPC.h"
#include "common.h"
#include "sysutil.h"
/**
 * 创建父子进程通信的socket
 * @param sess [description]
 */
void priv_sock_init(session_t *sess)
{
	int sockfds[2];
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) < 0)//创建unix域套接字
		ERR_EXIT("socketpair");

	sess->parent_fd = sockfds[0];
	sess->child_fd = sockfds[1];
}
/**
 * 关闭套接字
 * @param sess [description]
 */
void priv_sock_close(session_t *sess)
{
	if (sess->parent_fd != -1)
	{
		close(sess->parent_fd);
		sess->parent_fd = -1;
	}

	if (sess->child_fd != -1)
	{
		close(sess->child_fd);
		sess->child_fd = -1;
	}
}
/**
 * 父进程环境的初始化
 * @param sess [description]
 */
void priv_sock_set_parent_context(session_t *sess)
{
	if (sess->child_fd != -1)
	{//将子进程的套接字关闭
		close(sess->child_fd);
		sess->child_fd = -1;
	}
}
/**
 * 子进程环境的初始化
 * @param sess [description]
 */
void priv_sock_set_child_context(session_t *sess)
{
	if (sess->parent_fd != -1)
	{//将父进程的套接字关闭
		close(sess->parent_fd);
		sess->parent_fd = -1;
	}
}
/**
 * 发送命令，命令在头文件中定义
 * @param fd  [description]
 * @param cmd [description]
 */
void priv_sock_send_cmd(int fd, char cmd)
{
	int ret;
	ret = sysutil::writen(fd, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
	{
		FTPD_LOG(ERROR,"priv_sock_send_cmd error\n");
	}
}

char priv_sock_get_cmd(int fd)
{
	char res;
	int ret;
	ret = sysutil::readn(fd, &res, sizeof(res));
	if (ret == 0)
	{//说明ftp服务进程已经退出，那么nobody进程也应该退出
		printf("ftp process exit\n");
		exit(EXIT_SUCCESS);
	}
	if (ret != sizeof(res))
	{
		FTPD_LOG(ERROR,"priv_sock_get_cmd error\n");
	}

	return res;
}
/**
 * 发送应答，应答在头文件中定义
 * @param fd  [description]
 * @param res [description]
 */
void priv_sock_send_result(int fd, char res)
{
	int ret;
	ret = sysutil::writen(fd, &res, sizeof(res));
	if (ret != sizeof(res))
	{
		FTPD_LOG(ERROR,"priv_sock_send_result error\n");
	}
}

char priv_sock_get_result(int fd)
{
	char res;
	int ret;
	ret = sysutil::readn(fd, &res, sizeof(res));
	if (ret != sizeof(res))
	{
		FTPD_LOG(ERROR,"priv_sock_get_result error\n");
	}

	return res;
}
/**
 * 发送整数
 * @param fd      [description]
 * @param the_int [description]
 */
void priv_sock_send_int(int fd, int the_int)
{
	int ret;
	ret = sysutil::writen(fd, &the_int, sizeof(the_int));
	if (ret != sizeof(the_int))
	{
		FTPD_LOG(ERROR,"priv_sock_send_int error\n");
	}
}

int priv_sock_get_int(int fd)
{
	int the_int;
	int ret;
	ret = sysutil::readn(fd, &the_int, sizeof(the_int));
	if (ret != sizeof(the_int))
	{
		FTPD_LOG(ERROR,"priv_sock_get_int error\n");
	}

	return the_int;
}
/**
 * 发送字符串，先发送长度
 * @param fd  [description]
 * @param buf [description]
 * @param len 字符串的长度
 */
void priv_sock_send_buf(int fd, const char *buf, unsigned int len)
{
	priv_sock_send_int(fd, (int)len);
	int ret = sysutil::writen(fd, buf, len);
	if (ret != (int)len)
	{
		FTPD_LOG(ERROR,"priv_sock_send_buf error\n");
	}
}

void priv_sock_recv_buf(int fd, char *buf, unsigned int len)
{
	unsigned int recv_len = (unsigned int)priv_sock_get_int(fd);
	if (recv_len > len)
	{
		FTPD_LOG(ERROR,"priv_sock_recv_buf error\n");
	}

	int ret = sysutil::readn(fd, buf, recv_len);
	if (ret != (int)recv_len)
	{
		FTPD_LOG(ERROR,"priv_sock_recv_buf error\n");
	}
}
/**
 * 发送文件描述符
 * @param sock_fd unix域套接字fd
 * @param fd      要发送的fd
 */
void priv_sock_send_fd(int sock_fd, int fd)
{
	sysutil::send_fd(sock_fd, fd);
}

int priv_sock_recv_fd(int sock_fd)
{
	return sysutil::recv_fd(sock_fd);
}



