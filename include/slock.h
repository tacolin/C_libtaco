#ifndef _SIMPLE_LOCK_H_
#define _SIMPLE_LOCK_H_

#define atom_spin_lock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spin_unlock(ptr) __sync_lock_release(ptr)

#define atom_inc(ptr) __sync_add_and_fetch(ptr, 1)
#define atom_dec(ptr) __sync_sub_and_fetch(ptr, 1)

#define atom_sync() __sync_synchronize()

#define spin_lock(Q) atom_spin_lock(&Q->lock);
#define spin_unlock(Q) atom_spin_unlock(&Q->lock);

typedef struct rwlock
{
    int write;
    int read;

} tRwlock;

static inline void rwlock_init(struct rwlock* lock)
{
    lock->write = lock->read = 0;
}

static inline void rwlock_rlock(struct rwlock* lock)
{
    for (;;)
    {
        while (lock->write) atom_sync();

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

static inline void rwlock_wlock(struct rwlock* lock)
{
    atom_spin_lock(&lock->write);
    while (lock->write) atom_sync();
}

static inline void rwlock_wunlock(struct rwlock* lock)
{
    atom_spin_unlock(&lock->write);
}

static inline void rwlock_runlock(struct rwlock* lock)
{
    atom_dec(&lock->read);
}

#endif //_SIMPLE_LOCK_H_
