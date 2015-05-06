#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "fast_queue.h"

#define DEFAULT_DEPTH  (16)

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#define LOCK(q) atom_spinlock(&q->lock)
#define UNLOCK(q) atom_spinunlock(&q->lock)

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

struct fqueue
{
    int num;
    int head;
    int tail;
    int lock;

    void** data;

    void (*cleanfn)(void* ud);
};

struct fqueue* fqueue_create(void (*cleanfn)(void* ud))
{
    struct fqueue* fq = calloc(sizeof(struct fqueue), 1);
    CHECK_IF(fq == NULL, return NULL, "callc failed");

    fq->lock    = 0;
    fq->num     = DEFAULT_DEPTH;
    fq->head    = 0;
    fq->tail    = 0;
    fq->data    = calloc(sizeof(void*), fq->num);
    fq->cleanfn = cleanfn;
    return fq;
}

void fqueue_release(struct fqueue* fq)
{
    CHECK_IF(fq == NULL, return, "fq is null");

    LOCK(fq);
    {
        if (fq->cleanfn)
        {
            int head = fq->head;
            int tail = fq->tail;
            if (tail < head)
            {
                tail += fq->num;
            }

            int i;
            for (i=head; i<tail; i++)
            {
                fq->cleanfn(fq->data[i % fq->num]);
            }
        }
        free(fq->data);
    }
    UNLOCK(fq);
    free(fq);
    return;
}

static void _expand_fqueue(struct fqueue* fq)
{
    void** data = calloc(sizeof(void*), fq->num*2);
    CHECK_IF(data == NULL, return, "calloc failed");

    int i;
    int head = fq->head;
    for (i=0; i<fq->num; i++)
    {
        data[i] = fq->data[head % fq->num];
        head++;
    }
    free(fq->data);
    fq->data = data;
    fq->head = 0;
    fq->tail = fq->num;
    fq->num *= 2;
    return;
}

int fqueue_push(struct fqueue* fq, void* ud)
{
    CHECK_IF(fq == NULL, return FQUEUE_FAIL, "fq is null");
    CHECK_IF(ud == NULL, return FQUEUE_FAIL, "ud is null");

    LOCK(fq);
    {
        int tail = fq->tail;
        fq->data[tail] = ud;
        tail++;
        if (tail >= fq->num)
        {
            tail = 0;
        }

        if (tail == fq->head)
        {
            _expand_fqueue(fq);
        }
        else
        {
            fq->tail = tail;
        }
    }
    UNLOCK(fq);
    return FQUEUE_OK;
}

void* fqueue_pop(struct fqueue* fq)
{
    CHECK_IF(fq == NULL, return NULL, "fq is null");

    if (fq->head == fq->tail) return NULL; // EMPTY

    void* ret;
    LOCK(fq);
    {
        ret = fq->data[fq->head];
        fq->head++;
        if (fq->head == fq->num)
        {
            fq->head = 0;
        }
    }
    UNLOCK(fq);
    return ret;
}

int fqueue_empty(struct fqueue* fq)
{
    CHECK_IF(fq == NULL, return 1, "fq is null");
    return (fq->head == fq->tail);
}
