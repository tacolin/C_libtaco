#ifndef _SIMPLE_THREAD_H_
#define _SIMPLE_THREAD_H_

#include <pthread.h>

typedef struct tThread
{
    void (*func)(void* arg);
    void* arg;

} tThread;

typedef struct tThreadEv
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int flag;

} tThreadEv;

void thread_join(tThread* threads, int num);
void thread_ev_create(tThreadEv* ev);
void thread_ev_release(tThreadEv* ev);
void thread_ev_trigger(tThreadEv* ev);
void thread_ev_wait(tThreadEv* ev);

#endif //_SIMPLE_THREAD_H_
