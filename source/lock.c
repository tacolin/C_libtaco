#include "lock.h"

////////////////////////////////////////////////////////////////////////////////

int lock_init(tLock *lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_mutex_init(lock, NULL);
    check_if(ret != 0, return LOCK_FAIL, "pthread_mutex_init failed, ret = %d", ret);

    return LOCK_OK;
}

int lock_uninit(tLock *lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_mutex_destroy(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_mutext_destroy failed, ret = %d", ret);

    return LOCK_OK;
}

int lock_enter(tLock *lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_mutex_lock(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_mutex_lock failed, ret = %d", ret);

    return LOCK_OK;
}

int lock_exit(tLock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_mutex_unlock(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_mutex_unlock failed, ret = %d", ret);

    return LOCK_OK;
}

int lock_try(tLock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");
    int ret = pthread_mutex_trylock(lock);
    return (ret == 0) ? LOCK_OK : LOCK_FAIL;
}

////////////////////////////////////////////////////////////////////////////////

int rwlock_init(tRwlock *lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_rwlock_init(lock, NULL);
    check_if(ret != 0, return LOCK_FAIL, "pthread_rwlock_init failed, ret = %d", ret);

    return LOCK_OK;
}

int rwlock_uninit(tRwlock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_rwlock_destroy(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_rwlock_destroy failed, ret = %d", ret);

    return LOCK_OK;
}

int rwlock_enterRead(tRwlock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_rwlock_rdlock(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_rwlock_rdlock failed, ret = %d", ret);

    return LOCK_OK;
}

int rwlock_exitRead(tRwlock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_rwlock_unlock(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_rwlock_unlock failed, ret = %d", ret);

    return LOCK_OK;
}

int rwlock_tryRead(tRwlock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");
    int ret = pthread_rwlock_tryrdlock(lock);
    return (ret == 0) ? LOCK_OK : LOCK_FAIL;
}

int rwlock_enterWrite(tRwlock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_rwlock_wrlock(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_rwlock_wrlock failed, ret = %d", ret);

    return LOCK_OK;
}

int rwlock_exitWrite(tRwlock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");

    int ret = pthread_rwlock_unlock(lock);
    check_if(ret != 0, return LOCK_FAIL, "pthread_rwlock_unlock failed, ret = %d", ret);

    return LOCK_OK;
}

int rwlock_tryWrite(tRwlock* lock)
{
    check_if(lock == NULL, return LOCK_FAIL, "lock is null");
    int ret = pthread_rwlock_trywrlock(lock);
    return (ret == 0) ? LOCK_OK : LOCK_FAIL;
}
