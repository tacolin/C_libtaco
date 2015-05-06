#include <stdio.h>
#include "thread.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static void* _thread_routine(void* input)
{
    CHECK_IF(input == NULL, return NULL, "input is null");

    struct thread* t = (struct thread*)input;
    t->func(t->arg);
    return NULL;
}

void thread_join(struct thread* thread_array, int num)
{
    CHECK_IF(thread_array == NULL, return, "thread_array is null");
    CHECK_IF(num <= 0, return, "num = %d invalid", num);

    pthread_t tid[num];
    int i, chk;
    for (i=0; i<num; i++)
    {
        chk = pthread_create(&tid[i], NULL, _thread_routine, &thread_array[i]);
        CHECK_IF(chk != 0, return, "pthread_create thread_array[%d] failed", i);
    }

    for (i=0; i<num; i++)
    {
        pthread_join(tid[i], NULL);
    }
    return;
}

void thread_ev_create(struct thread_event* ev)
{
    CHECK_IF(ev == NULL, return, "ev is null");

    pthread_mutex_init(&ev->mutex, NULL);
    pthread_cond_init(&ev->cond, NULL);
    ev->flag = 0;
}

void thread_ev_release(struct thread_event* ev)
{
    CHECK_IF(ev == NULL, return, "ev is null");

    pthread_mutex_destroy(&ev->mutex);
    pthread_cond_destroy(&ev->cond);
}

void thread_ev_trigger(struct thread_event* ev)
{
    CHECK_IF(ev == NULL, return, "ev is null");

    pthread_mutex_lock(&ev->mutex);
    ev->flag = 1;
    pthread_mutex_unlock(&ev->mutex);
    pthread_cond_signal(&ev->cond);
}

void thread_ev_wait(struct thread_event* ev)
{
    CHECK_IF(ev == NULL, return, "ev is null");

    pthread_mutex_lock(&ev->mutex);
    while (ev->flag == 0)
    {
        pthread_cond_wait(&ev->cond, &ev->mutex);
    }
    ev->flag = 0;
    pthread_mutex_unlock(&ev->mutex);
}
