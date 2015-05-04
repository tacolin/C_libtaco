#include "basic.h"
#include "simple_queue.h"

#define SQUEUE_DEFAULT_SIZE (16)

#define GET_OBJ(queue, i) (queue->objs[(i % queue->num)])

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

int squeue_init(tSqueue* queue, tSqueueCleanFn cleanfn)
{
    check_if(queue == NULL, return SQUEUE_FAIL, "queue is null");

    queue->lock    = 0;
    queue->num     = SQUEUE_DEFAULT_SIZE;
    queue->head    = 0;
    queue->tail    = 0;
    queue->cleanfn = cleanfn;
    queue->objs    = calloc(sizeof(void*), SQUEUE_DEFAULT_SIZE);

    return SQUEUE_OK;
}

void squeue_clean(tSqueue* queue)
{
    check_if(queue == NULL, return, "queue is null");

    atom_spinlock(&queue->lock);

    if (queue->cleanfn)
    {
        int head = queue->head;
        int tail = queue->tail;
        if (tail < head)
        {
            tail += queue->num;
        }
        int i;
        void* obj;
        for (i=head; i<tail; i++)
        {
            obj = GET_OBJ(queue, i);
            queue->cleanfn(obj);
        }
    }

    free(queue->objs);

    atom_spinunlock(&queue->lock);
    return;
}

int squeue_push(tSqueue* queue, void* content)
{
    check_if(queue == NULL, return SQUEUE_FAIL, "queue is null");

    atom_spinlock(&queue->lock);

    int tail = queue->tail;

    queue->objs[tail] = content;

    tail++;

    if (tail >= queue->num)
    {
        tail = 0;
    }

    if (tail == queue->head)
    {
        // expand queue
        int new_num = queue->num * 2;
        void** new_objs = calloc(sizeof(void*), new_num);
        int i;
        int head = queue->head;
        for (i=0; i<queue->num; i++)
        {
            new_objs[i] = GET_OBJ(queue, head);
            head++;
        }
        free(queue->objs);

        queue->objs = new_objs;
        queue->head = 0;
        queue->tail = queue->num;
        queue->num  = new_num;
    }
    else
    {
        queue->tail = tail;
    }
    atom_spinunlock(&queue->lock);

    return SQUEUE_OK;
}

void* squeue_pop(tSqueue* queue)
{
    check_if(queue == NULL, return NULL, "queue is null");

    atom_spinlock(&queue->lock);
    if (squeue_isEmpty(queue))
    {
        atom_spinunlock(&queue->lock);
        return NULL;
    }

    void* ret = queue->objs[queue->head];
    queue->head++;

    if (queue->head == queue->num)
    {
        queue->head = 0;
    }

    atom_spinunlock(&queue->lock);
    return ret;
}

int squeue_isEmpty(tSqueue* queue)
{
    check_if(queue == NULL, return 1, "queue is null");
    return (queue->head == queue->tail);
}
