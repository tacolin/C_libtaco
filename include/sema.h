#ifndef _SEM_H_
#define _SEM_H_

#include "basic.h"
#include <semaphore.h>

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    SEM_OK = 0,
    SEM_ERROR = -1

} tSemStatus;

typedef sem_t tSem;

////////////////////////////////////////////////////////////////////////////////

static inline tSemStatus sema_init(tSem *sem, int count)
{
    check_if(sem == NULL, return SEM_ERROR, "sem is null");
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

static inline tSemStatus sema_dec(tSem *sem)
{
    check_if(sem == NULL, return SEM_ERROR, "sem is null");
    if (0 == sem_wait(sem)) return SEM_OK;
    else return SEM_ERROR;
}


#endif //_SEM_H_
