#ifndef _FRW_LOCK_H_
#define _FRW_LOCK_H_

#include "basic.h"

////////////////////////////////////////////////////////////////////////////////

#define atom_sync() __sync_synchronize()
#define atom_inc(ptr) __sync_add_and_fetch(ptr, 1)
#define atom_dec(ptr) __sync_sub_and_fetch(ptr, 1)
#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

////////////////////////////////////////////////////////////////////////////////

typedef struct tFrwlock
{
    int write;
    int read;

} tFrwlock;

static inline void frwlock_init(tFrwlock* lock)
{
    lock->write = lock->read = 0;
}

static inline void frwlock_rlock(tFrwlock* lock)
{
    for (;;)
    {
        while (lock->write)
        {
            atom_sync();
        }

        atom_inc(&lock->read);

        if (lock->write)
        {
            atom_dec(&lock->read);
        }
        else
        {
            break;
        }
    }
}

static inline void frwlock_runlock(tFrwlock* lock)
{
    atom_dec(&lock->read);
}

static inline void frwlock_wlock(tFrwlock* lock)
{
    atom_spinlock(&lock->write);
    while (lock->read)
    {
        atom_sync();
    }
}

static inline void frwlock_wunlock(tFrwlock* lock)
{
    atom_spinunlock(&lock->write);
}

#endif //_FRW_LOCK_H_
