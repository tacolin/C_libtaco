#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#define atom_sync() __sync_synchronize()
#define atom_inc(ptr) __sync_add_and_fetch(ptr, 1)
#define atom_dec(ptr) __sync_sub_and_fetch(ptr, 1)
#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

struct rwlock
{
    int write;
    int read;
};

static inline void rwlock_init(struct rwlock* lock)
{
    lock->write = lock->read = 0;
}

static inline void rwlock_rlock(struct rwlock* lock)
{
    while (1)
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

static inline void rwlock_runlock(struct rwlock* lock)
{
    atom_dec(&lock->read);
}

static inline void rwlock_wlock(struct rwlock* lock)
{
    atom_spinlock(&lock->write);
    while (lock->read)
    {
        atom_sync();
    }
}

static inline void rwlock_wunlock(struct rwlock* lock)
{
    atom_spinunlock(&lock->write);
}

#endif //_RWLOCK_H_
