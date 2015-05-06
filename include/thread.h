#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>

struct thread
{
    void (*func)(void* arg);
    void* arg;
};

struct thread_event
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int flag;
};

void thread_join(struct thread* thread_array, int num);
void thread_ev_create(struct thread_event* ev);
void thread_ev_release(struct thread_event* ev);
void thread_ev_trigger(struct thread_event* ev);
void thread_ev_wait(struct thread_event* ev);
#endif //_THREAD_H_
