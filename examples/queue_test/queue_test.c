#include <stdint.h>
#include "basic.h"
#include "queue.h"
#include "thread.h"

static void _cleanFn(void* content)
{
    dprint("content = %p", content);
}

static void _producer(void* arg)
{
    struct queue* q = (struct queue*)arg;
    int i;
    for (i=0; i<100; i++)
    {
        queue_push(q, (void*)(intptr_t)(i+1));
        dprint("i = %d data = %p", i, (void*)(intptr_t)(i+1));
        usleep(10);
    }
}

static void _consumer(void* arg)
{
    struct queue* q = (struct queue*)arg;
    int i;
    void* data;
    for (i=0; i<100; i++)
    {
        data = queue_pop(q);
        dprint("i = %d data = %p", i, data);
        usleep(100);
    }
}

int main(int argc, char const *argv[])
{
    struct queue q;
    queue_init(&q, -1, NULL, 0);

    queue_push(&q, (void*)(intptr_t)1);
    queue_push(&q, (void*)(intptr_t)2);
    queue_push(&q, (void*)(intptr_t)3);
    queue_push(&q, (void*)(intptr_t)4);
    queue_push(&q, (void*)(intptr_t)5);
    queue_push(&q, (void*)(intptr_t)6);

    dprint("queue_num = %d", queue_num(&q));

    void* data;
    QUEUE_FOREACH(&q, data)
    {
        dprint("data = %p", data);
    }
    queue_clean(&q);

    queue_init(&q, 3, _cleanFn, 0);

    queue_push(&q, (void*)(intptr_t)10);
    queue_push(&q, (void*)(intptr_t)20);
    queue_push(&q, (void*)(intptr_t)30);
    int chk = queue_push(&q, (void*)(intptr_t)40);
    dprint("chk = %d", chk);

    dprint("queue_num = %d", queue_num(&q));

    QUEUE_FOREACH(&q, data)
    {
        dprint("data = %p", data);
    }

    queue_clean(&q);

    queue_init(&q, 5, NULL, QUEUE_FLAG_PUSH_BLOCK | QUEUE_FLAG_POP_BLOCK);
    // queue_init(&q, 5, NULL, QUEUE_FLAG_POP_BLOCK);
    // queue_init(&q, 5, NULL, QUEUE_FLAG_PUSH_BLOCK);

    struct thread t[2] = {
        {_producer, &q},
        {_consumer, &q}
    };

    thread_join(t, 2);

    queue_clean(&q);

    dprint("ok");
    return 0;
}
