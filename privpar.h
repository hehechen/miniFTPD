#ifndef PRIVPARH
#define PRIVPAR_H

#include "session.h"
#include "parseconfig.h"

class PrivPar
{
public:
	PrivPar();
	~PrivPar()	{}
	void handle_parent(session_t *sess);
private:
	ParseConfig *parseconfig;
	void privop_pasv_get_data_sock(session_t *sess);
	void privop_pasv_active(session_t *sess);
	void privop_pasv_listen(session_t *sess);
	void privop_pasv_accept(session_t *sess);
};


#endif // PRIVPAR_H


