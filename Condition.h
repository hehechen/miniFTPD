#include "MutexLock.h"

class Condition
{
public:
	explicit Condition(MutexLock &mutex):mutex_(mutex)
	{
		pthread_cond_init(&pcond_,NULL);
	}
	~Condition()	{	pthread_cond_destroy(&pcond_);	}
	void wait()	{	pthread_cond_wait(&pcond_,mutex_.getMutex());	}
	void notify()	{	pthread_cond_signal(&pcond_);	}

private:
	MutexLock &mutex_;
	pthread_cond_t pcond_;

};