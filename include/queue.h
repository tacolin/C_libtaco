#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "basic.h"
#include <pthread.h>
#include <semaphore.h>

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    QUEUE_OK = 0,
    QUEUE_ERROR = -1

} tQueueStatus;

typedef enum
{
    QUEUE_SUSPEND = 1,
    QUEUE_UNSUSPEND = 0

} tQueueSuspend;

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

tQueueStatus queue_init(tQueue *queue, int max_queue_depth,
                        tQueueContentCleanFn clean_func,
                        tQueueSuspend is_put_suspend,
                        tQueueSuspend is_get_suspend);

void queue_clean(tQueue *queue);

tQueueStatus queue_push(tQueue* queue, void* content);
void* queue_pop(tQueue* queue);

int queue_length(tQueue* queue);

#endif //_QUEUE_H_
