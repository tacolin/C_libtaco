#include "simple_thread.h"
#include "basic.h"

static void* _threadRoutine(void* input)
{
    tSthread* t = (tSthread*)input;
    t->func(t->arg);
    return NULL;
}

void sthread_join(tSthread* threads, int num)
{
    pthread_t tid[num];
    int i, chk;
    for (i=0; i<num; i++)
    {
        chk = pthread_create(&tid[i], NULL, _threadRoutine, &threads[i]);
        check_if(chk != 0, return, "pthread_create threads[%d] failed", i);
    }

    for (i=0; i<num; i++)
    {
        pthread_join(tid[i], NULL);
    }

    return;
}

void sthread_ev_create(tSthreadEv* ev)
{
    pthread_mutex_init(&ev->mutex, NULL);
    pthread_cond_init(&ev->cond, NULL);
    ev->flag = 0;
}

void sthread_ev_release(tSthreadEv* ev)
{
    pthread_mutex_destroy(&ev->mutex);
    pthread_cond_destroy(&ev->cond);
}

void sthread_ev_trigger(tSthreadEv* ev)
{
    pthread_mutex_lock(&ev->mutex);
    ev->flag = 1;
    pthread_mutex_unlock(&ev->mutex);
    pthread_cond_signal(&ev->cond);
}

void sthread_ev_wait(tSthreadEv* ev)
{
    pthread_mutex_lock(&ev->mutex);
    while (ev->flag == 0)
    {
        pthread_cond_wait(&ev->cond, &ev->mutex);
    }
    ev->flag = 0;
    pthread_mutex_unlock(&ev->mutex);
}
