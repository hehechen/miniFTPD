#include "session.h"
#include "ftpproto.h"
#include "privpar.h"
#include "ftpdIPC.h"
void begin_session(session_t* sess)
{
	priv_sock_init(sess);
	PrivPar privPar;
	pid_t pid;
	if((pid = fork())<0)
		FTPD_LOG(ERROR,"fork error");
	if(pid == 0)
	{
		priv_sock_set_child_context(sess);
		handle_child(sess);
	}
	else
	{
		priv_sock_set_parent_context(sess);
		privPar.handle_parent(sess);
	}
}