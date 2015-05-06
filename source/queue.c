#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#define LOCK(q) atom_spinlock(&q->lock)
#define UNLOCK(q) atom_spinunlock(&q->lock)

int queue_init(struct queue* q, int depth, void (*cleanfn)(void*), int flag)
{
    CHECK_IF(q == NULL, return QUEUE_FAIL, "q is null");
    CHECK_IF(depth == 0, return QUEUE_FAIL, "depth is %d", depth);

    q->head = NULL;
    q->tail = NULL;
    q->depth = depth;
    q->num = 0;
    sem_init(&(q->num_sem), 0, 0);
    q->cleanfn = cleanfn;
    q->lock = 0;
    q->flag = flag;
    if (depth > 0)
    {
        sem_init(&(q->empty_sem), 0, depth);
    }
    else
    {
        sem_init(&(q->empty_sem), 0, 0);
        q->flag &= ~QUEUE_FLAG_PUSH_BLOCK;
    }
    return QUEUE_OK;
}

void queue_clean(struct queue* q)
{
    CHECK_IF(q == NULL, return, "q is null");

    LOCK(q);
    struct queue_node* node = q->head;
    struct queue_node* next;
    while (node)
    {
        next = node->next;
        if (q->cleanfn)
        {
            q->cleanfn(node->data);
        }
        free(node);
        node = next;
    }
    q->num  = 0;
    q->head = NULL;
    q->tail = NULL;
    sem_init(&(q->num_sem), 0, 0);
    if (q->depth > 0)
    {
        sem_init(&(q->empty_sem), 0, q->depth);
    }
    else
    {
        sem_init(&(q->empty_sem), 0, 0);
    }
    UNLOCK(q);
    return;
}

int queue_push(struct queue* q, void* data)
{
    CHECK_IF(q == NULL, return QUEUE_FAIL, "q is null");
    CHECK_IF(data == NULL, return QUEUE_FAIL, "data is null");

    if (q->flag & QUEUE_FLAG_PUSH_BLOCK)
    {
        sem_wait(&(q->empty_sem));
    }
    else
    {
        if ((q->depth > 0) && (q->num >= q->depth)) // queue full
        {
            return QUEUE_FAIL;
        }
    }

    struct queue_node* node = calloc(sizeof(struct queue_node), 1);
    node->data = data;

    LOCK(q);
    if (q->tail)
    {
        q->tail->next = node;
        q->tail       = node;
    }
    else
    {
        q->tail = node;
        q->head = node;
    }

    q->num++;

    if (q->flag & QUEUE_FLAG_POP_BLOCK)
    {
        sem_post(&(q->num_sem));
    }
    UNLOCK(q);
    return QUEUE_OK;
}

void* queue_pop(struct queue* q)
{
    CHECK_IF(q == NULL, return NULL, "q is null");

    if (q->flag & QUEUE_FLAG_POP_BLOCK)
    {
        sem_wait(&(q->num_sem));
    }
    else if (q->num <= 0)
    {
        CHECK_IF(q->head, return NULL, "q is empty but head exists");
        CHECK_IF(q->tail, return NULL, "q is empty but tail exists");
        return NULL;
    }

    CHECK_IF(q->head == NULL, return NULL, "num = %d, but head is NULL", q->num);
    CHECK_IF(q->tail == NULL, return NULL, "num = %d, but tail is NULL", q->num);

    LOCK(q);
    struct queue_node *node = q->head;
    if (node->next)
    {
        q->head = node->next;
    }
    else
    {
        q->head = NULL;
        q->tail = NULL;
    }
    q->num--;

    if (q->flag & QUEUE_FLAG_PUSH_BLOCK)
    {
        sem_post(&(q->empty_sem));
    }
    UNLOCK(q);

    void* data = node->data;
    free(node);
    return data;
}

int queue_num(struct queue* q)
{
    CHECK_IF(q == NULL, return -1, "q is null");
    return q->num;
}
