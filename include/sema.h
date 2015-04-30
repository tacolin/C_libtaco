#ifndef _SEM_H_
#define _SEM_H_

#include "basic.h"
#include <semaphore.h>

////////////////////////////////////////////////////////////////////////////////

#define SEM_OK (0)
#define SEM_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef sem_t tSem;

////////////////////////////////////////////////////////////////////////////////

static inline int sema_init(tSem *sem, int count)
{
    check_if(sem == NULL, return SEM_FAIL, "sem is null");
    sem_init(sem, 0, count);
    return SEM_OK;
}

static inline void sema_uninit(tSem *sem)
{
    check_if(sem == NULL, return, "sem is null");
    sem_destroy(sem);
    return;
}

static inline void sema_inc(tSem *sem)
{
    check_if(sem == NULL, return, "sem is null");
    sem_post(sem);
    return;
}

static inline int sema_dec(tSem *sem)
{
    check_if(sem == NULL, return SEM_FAIL, "sem is null");
    if (0 == sem_wait(sem)) return SEM_OK;
    else return SEM_FAIL;
}


#endif //_SEM_H_
