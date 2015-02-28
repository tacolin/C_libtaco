#ifndef _LOCK_H_
#define _LOCK_H_

#include "basic.h"
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////

typedef pthread_mutex_t tLock;

typedef enum
{
    LOCK_OK = 0,
    LOCK_ERROR = -1

} tLockStatus;

////////////////////////////////////////////////////////////////////////////////

static inline tLockStatus lock_init(tLock *lock)
{
    check_if(lock == NULL, return LOCK_ERROR, "lock is null");
    int ret = pthread_mutex_init(lock, NULL);
    return (ret == 0) ? LOCK_OK : LOCK_ERROR;
}

static inline void lock_uninit(tLock *lock)
{
    check_if(lock == NULL, return, "lock is null");
    return;
}

static inline tLockStatus lock_enter(tLock *lock)
{
    check_if(lock == NULL, return LOCK_ERROR, "lock is null");
    int ret = pthread_mutex_lock(lock);
    return (ret == 0) ? LOCK_OK : LOCK_ERROR;
}

static inline void lock_exit(tLock* lock)
{
    check_if(lock == NULL, return, "lock is null");
    pthread_mutex_unlock(lock);
    return;
}

#endif //_LOCK_H_
