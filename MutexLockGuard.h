#ifndef MUTEX_LOCK_GUARD_H
#define MUTEX_LOCK_GUARD_H

#include <pthread.h>
#include "MutexLock.h"
#include "common.h"
///@author chen
///栈上对象，封装线程互斥锁的加锁和解锁
class MutexLockGuard
{
public:
    MutexLockGuard(MutexLock &mutex):mutex(mutex)   {  mutex.lock(); }
    ~MutexLockGuard()   {  mutex.unlock(); }
private:
    MutexLock &mutex;
};

//这个宏的作用是防止MutexGuard(mutex)产生临时对象从而没有锁住临界区
#define MutexLockGuard(x) static_assert(false,"missing mutex guard var name")


#endif // MUTEX_LOCK_GUARD_H
