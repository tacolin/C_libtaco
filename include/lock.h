#ifndef _LOCK_H_
#define _LOCK_H_

#include "basic.h"
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////

typedef pthread_mutex_t tLock;

typedef pthread_rwlock_t tRwlock;

typedef enum
{
    LOCK_OK = 0,
    LOCK_ERROR = -1

} tLockStatus;

////////////////////////////////////////////////////////////////////////////////

tLockStatus lock_init(tLock *lock);
tLockStatus lock_uninit(tLock *lock);

tLockStatus lock_enter(tLock *lock);
tLockStatus lock_exit(tLock* lock);

tLockStatus lock_try(tLock* lock);

////////////////////////////////////////////////////////////////////////////////

tLockStatus rwlock_init(tRwlock *lock);
tLockStatus rwlock_uninit(tRwlock* lock);

tLockStatus rwlock_enterRead(tRwlock* lock);
tLockStatus rwlock_exitRead(tRwlock* lock);
tLockStatus rwlock_tryRead(tRwlock* lock);

tLockStatus rwlock_enterWrite(tRwlock* lock);
tLockStatus rwlock_exitWrite(tRwlock* lock);
tLockStatus rwlock_tryWrite(tRwlock* lock);

#endif //_LOCK_H_
