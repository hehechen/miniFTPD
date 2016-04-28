#include "parseconfig.h"
#include "str_tool.h"
//在进入主函数前就初始化，保证线程安全	
ParseConfig *ParseConfig::m_instance = new ParseConfig();
ParseConfig *ParseConfig::getInstance()
{
	atexit(destroy);
	return m_instance;
}

ParseConfig::ParseConfig():CONFILE("chenftpd.conf"),pasv_active(1),port_active(1),listen_address(NULL),
					local_umask(077)
	{
		FTPD_LOG(DEBUG,"ParseConfig construct");
		unint_config["listen_port"] = 21;
		unint_config["max_clients"] = 2000;
		unint_config["max_per_ip"] = 50;
		unint_config["accept_timeout"] = 60;
		unint_config["connect_timeout"] = 60;
		unint_config["idle_session_timeout"] = 300;
		unint_config["data_connection_timeout"] = 300;
		unint_config["upload_max_rate"] = 0;
		unint_config["download_max_rate"] = 0;
	}
ParseConfig::~ParseConfig(){
	FTPD_LOG(DEBUG,"ParseConfig desconstruct");
}

void ParseConfig::destroy()
{
	if(m_instance)
	{
		delete m_instance;
		m_instance = nullptr;
		FTPD_LOG(DEBUG,"ParseConfig destroy");
	}
}
/**
 * 加载配置文件
 * 成功返回0 失败返回-1
 */
void ParseConfig::loadfile()
{
	FILE* fp = fopen(CONFILE,"r");	//只读模式打开配置文件
	if(fp == NULL)
		FTPD_LOG(ERROR,"open confile error");
	//以只读方式打开文件CONF_FILE，文件必须已经存在
	if ((fp = fopen(CONFILE,"r")) == NULL)
		FTPD_LOG(ERROR,"Can't load the configure file %s",CONFILE);
	
	int linenumber = 0;//这个参数用于记录配置文件的行数，便于在配置文件出错时打印提醒
	char* line;//行指针
	char key[128] = {0};//保存命令的key
	char value[128] = {0};//保存命令的value
	char linebuf[MAX_CONF_LEN+1];//保存读取到的行
	while(fgets(linebuf,sizeof(linebuf),fp))
	{
		++linenumber;//行数++
		line = str_delspace(linebuf);//去除行首和行尾的空格、\r\n
		if (line[0] == '#' || line[0] == '\0')
		{
			continue;//注释或空行直接跳过
		}
		str_split(line,key,value,'=');//根据等号切割
		strcpy(key,str_delspace(key));//去除空格
		strcpy(value,str_delspace(value));//去除空格
		if(0 == strlen(value))
		{//某些key没有配置value,提示key和行号
			FTPD_LOG(DEBUG,"missing value in %s for %s locate line %d",CONFILE,key,linenumber);
		}
		//这里只会在程序执行的时候执行一次，比较次数也不算多，姑且用if else 
		//strcasecmp忽略大小写比较字符串
		if(strcasecmp(key, "pasv_enable") == 0)//被动模式
		{ 
            pasv_active = config_pasv_port(key,value,linenumber);
            FTPD_LOG(DEBUG,"value for pasv_enable is %d",pasv_active);
        }
        else if(strcasecmp(key, "port_enable") == 0)//主动模式
        {
        	port_active = config_pasv_port(key,value,linenumber); 
        	FTPD_LOG(DEBUG,"value for port_enable is %d",port_active);
        }
        else if(strcasecmp(key, "local_umask") == 0)//权限掩码
        {//8进制字符串转化为unsigned int
        	local_umask = str_octal_to_uint(value);
        	FTPD_LOG(DEBUG,"value for local_umask is %u(Decimal system)",local_umask);
        }
        else if(strcasecmp(key, "listen_address") == 0)//ip地址
        {
        	listen_address = strdup(value);
        	FTPD_LOG(DEBUG,"value for listen_address is %s",listen_address);
        }
        else
        {
            //配置所有unsigned int 类型的命令
            unsigned int val = atoi(value);
            unint_config[key] = val;
            FTPD_LOG(DEBUG,"value for %s is %d",key,val);
        }
	}  
    //关闭配置文件
	fclose(fp);
}

bool ParseConfig::config_pasv_port(char* key,char* value,int linenumber)
{
	 bool trueorfalse;
	 if (strcasecmp(value,"YES") == 0 || strcasecmp(value,"TRUE") == 0 || 
	    	strcasecmp(value,"1") == 0)
	 {//为真
	  	trueorfalse = true;
	 }
	 else if (strcasecmp(value,"NO") == 0 || strcasecmp(value,"FALSE") == 0 ||
	  	strcasecmp(value,"0") == 0)
	 {//为假
	   trueorfalse = false;
	 }
	 else//没有配置可以使用默认值，配置错了程序最好退出
	 {
	  	FTPD_LOG(ERROR,"value in %s [line: %d] for %s not support",CONFILE,linenumber,key);
	 }
	 return trueorfalse;
}

//获取配置数据
bool ParseConfig::get_pasv_active()
{
	return pasv_active;
}

bool ParseConfig::get_port_active()
{
	return port_active;
}
unsigned int ParseConfig::get_local_umask()
{
	return local_umask;
}
char* ParseConfig::get_listen_address()
{
	return listen_address;
}
unsigned int ParseConfig::get_listen_port()
{
	return unint_config["listen_port"];
}
unsigned int ParseConfig::get_max_clients()
{
	return unint_config["max_clients"];
}
unsigned int ParseConfig::get_max_per_ip()
{
	return unint_config["max_per_ip"];
}
unsigned int ParseConfig::get_accept_timeout()
{
	return unint_config["accept_timeout"];
}
unsigned int ParseConfig::get_connect_timeout()
{
	return unint_config["connect_timeout"];
}
unsigned int ParseConfig::get_idle_session_timeout()
{
	return unint_config["idle_session_timeout"];
}
unsigned int ParseConfig::get_data_connection_timeout()
{
	return unint_config["data_connection_timeout"];
}
unsigned int ParseConfig::get_upload_max_rate()
{
	return unint_config["upload_max_rate"];
}
unsigned int ParseConfig::get_download_max_rate()
{
	return unint_config["download_max_rate"];
}





