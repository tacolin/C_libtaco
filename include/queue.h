#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <semaphore.h>

#define QUEUE_OK (0)
#define QUEUE_FAIL (-1)

#define QUEUE_FLAG_PUSH_BLOCK (0x0001)
#define QUEUE_FLAG_POP_BLOCK  (0x0010)

#define QUEUE_FOREACH(pqueue, _data) for (_data = queue_pop(pqueue); _data; _data = queue_pop(pqueue))

struct queue_node
{
    struct queue_node* next;
    void* data;
};

struct queue
{
    struct queue_node* head;
    struct queue_node* tail;

    int depth;
    int num;

    sem_t num_sem;
    sem_t empty_sem;
    int lock;
    int flag;

    void (*cleanfn)(void* data);
};

int queue_init(struct queue* q, int depth, void (*cleanfn)(void*), int flag);
void queue_clean(struct queue* q);

int queue_push(struct queue* q, void *data);
void* queue_pop(struct queue* q);

int queue_num(struct queue* q);
#endif //_QUEUE_H_
