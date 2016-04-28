//解析配置文件的类
#ifndef _PARSECONFIH_H
#define _PARSECONFIH_H

#include "common.h"
#include <map>
#include <string>
using namespace std;
//饿汉模式单例类
class ParseConfig
{
public:
	//返回单例对象
	static ParseConfig *getInstance();
	void loadfile();	//加载配置文件
	//获取配置数据
	bool get_pasv_active();
	bool get_port_active();
	unsigned int get_listen_port();
	unsigned int get_max_clients();
	unsigned int get_max_per_ip();
	unsigned int get_accept_timeout();
	unsigned int get_connect_timeout();
	unsigned int get_idle_session_timeout();
	unsigned int get_data_connection_timeout();
	unsigned int get_local_umask();
	unsigned int get_upload_max_rate();
	unsigned int get_download_max_rate();
	char* get_listen_address();

private:
	static ParseConfig *m_instance;
	static void destroy();	//销毁该单例
	const char* CONFILE;//配置文件的文件名
	static const int MAX_CONF_LEN = 1024;
	ParseConfig();
	~ParseConfig();
	bool config_pasv_port(char* key,char* value,int linenumber);

	bool pasv_active;//被动模式
	bool port_active;//主动模式
	char* listen_address;//监听地址
	unsigned int local_umask;//掩码，这个是八进制，比较特殊

	map<string,unsigned int> unint_config;
/**** unsigned int型的命令都放在unint_config这个map里
	unsigned int listen_port;//监听端口 
	unsigned int max_clients;//最大连接数
	unsigned int max_per_ip;//同个ip的最大连接数
	unsigned int accept_timeout;//accept的等待时间
	unsigned int connect_timeout;//connect的等待时间
	unsigned int idle_session_timeout;//空闲超时
	unsigned int data_connection_timeout;// 数据连接断开时间
	unsigned int upload_max_rate;//上传最大速率
	unsigned int download_max_rate;//下载最大速率
	**/
};

#endif