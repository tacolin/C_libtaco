#ifndef _SIMPLE_THREAD_H_
#define _SIMPLE_THREAD_H_

#include <pthread.h>

typedef struct tSthread
{
    void (*func)(void* arg);
    void* arg;

} tSthread;

typedef struct tSthreadEv
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int flag;

} tSthreadEv;

void sthread_join(tSthread* threads, int num);
void sthread_ev_create(tSthreadEv* ev);
void sthread_ev_release(tSthreadEv* ev);
void sthread_ev_trigger(tSthreadEv* ev);
void sthread_ev_wait(tSthreadEv* ev);

#endif //_SIMPLE_THREAD_H_
