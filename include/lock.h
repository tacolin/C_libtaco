#ifndef _LOCK_H_
#define _LOCK_H_

#include "basic.h"
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////

#define LOCK_OK (0)
#define LOCK_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef pthread_mutex_t tLock;

typedef pthread_rwlock_t tRwlock;

////////////////////////////////////////////////////////////////////////////////

int lock_init(tLock *lock);
int lock_uninit(tLock *lock);

int lock_enter(tLock *lock);
int lock_exit(tLock* lock);

int lock_try(tLock* lock);

////////////////////////////////////////////////////////////////////////////////

int rwlock_init(tRwlock *lock);
int rwlock_uninit(tRwlock* lock);

int rwlock_enterRead(tRwlock* lock);
int rwlock_exitRead(tRwlock* lock);
int rwlock_tryRead(tRwlock* lock);

int rwlock_enterWrite(tRwlock* lock);
int rwlock_exitWrite(tRwlock* lock);
int rwlock_tryWrite(tRwlock* lock);

#endif //_LOCK_H_
