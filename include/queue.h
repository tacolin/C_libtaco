#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "basic.h"
#include <pthread.h>
#include <semaphore.h>

////////////////////////////////////////////////////////////////////////////////

#define QUEUE_OK   0
#define QUEUE_FAIL -1

#define QUEUE_PUT_SUSPEND   1
#define QUEUE_PUT_UNSUSPEND 0

#define QUEUE_GET_SUSPEND   1
#define QUEUE_GET_UNSUSPEND 0

////////////////////////////////////////////////////////////////////////////////

typedef struct tQueueObj
{
    void* content;
    struct tQueueObj* next_obj;

} tQueueObj;

typedef void (*tQueueContentCleanFn)(void* obj_content);

typedef struct tQueue
{
    int max_obj_num;
    int curr_obj_num;

    tQueueObj *head;
    tQueueObj *tail;

    sem_t obj_sem;
    sem_t empty_sem;

    int is_put_suspend;
    int is_get_suspend;

    pthread_mutex_t lock;

    tQueueContentCleanFn cleanfn;

} tQueue;

////////////////////////////////////////////////////////////////////////////////

int queue_init(tQueue *queue, int max_queue_depth,
               tQueueContentCleanFn clean_func,
               int is_put_suspend, int is_get_suspend);

void queue_clean(tQueue *queue);

int   queue_put(tQueue* queue, void* content);
void* queue_get(tQueue* queue);

#endif //_QUEUE_H_
