#ifndef FTPPROTO_H
#define FTPPROTO_H
#include <map>
#include <functional>
#include "session.h"

using namespace std;
//应答函数
void ftp_reply(session_t *sess, int status, const char *text);
void ftp_lreply(session_t* sess,int status,const char* text);

// void handle_alarm_timeout(int sig);
// void handle_sigalrm(int sig);
// void handle_sigurg(int sig);
// void start_cmdio_alarm(void);
// void start_data_alarm(void);

// void check_abor(session_t *sess);
//列出目录
int list_common(session_t *sess, int detail);

//void limit_rate(session_t *sess, int bytes_transfered, int is_upload);
//上传文件
void upload_common(session_t *sess, int is_append);

int get_port_fd(session_t *sess);
int get_pasv_fd(session_t *sess);
//创建数据连接
int get_transfer_fd(session_t *sess);
int port_active(session_t *sess);
int pasv_active(session_t *sess);


//命令与处理函数的映射
extern map<string,function<void (session_t *)> > cmd_handler;
void handle_child(session_t *sess); 
//int list_common(void); 
void ftp_reply(session_t *sess, int status, const char *text); 

#endif // FTPPROTO_H
