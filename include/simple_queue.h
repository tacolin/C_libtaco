#ifndef _SIMPLE_QUEUE_H_
#define _SIMPLE_QUEUE_H_

#define SQUEUE_OK (0)
#define SQUEUE_FAIL (-1)

typedef void (*tSqueueCleanFn)(void* content);

typedef struct tSqueue
{
    int lock;
    int num;
    int head;
    int tail;
    void** objs;

    tSqueueCleanFn cleanfn;

} tSqueue;

int squeue_init(tSqueue* queue, tSqueueCleanFn cleanfn);
void squeue_clean(tSqueue* queue);

int squeue_push(tSqueue* queue, void* content);
void* squeue_pop(tSqueue* queue);

int squeue_isEmpty(tSqueue* queue);

#endif //_SIMPLE_QUEUE_H_
