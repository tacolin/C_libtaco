#include <stdint.h>
#include "basic.h"

#include "fast_queue.h"
#include "thread.h"

static void _cleanFn(void* content)
{
    dprint("content = %p", content);
}

static void _producer(void* arg)
{
    struct fqueue* q = (struct fqueue*)arg;
    int i;
    for (i=0; i<100; i++)
    {
        fqueue_push(q, (void*)(intptr_t)(i+1));
        dprint("i = %d data = %p", i, (void*)(intptr_t)(i+1));
        usleep(10);
    }
}

static void _consumer(void* arg)
{
    struct fqueue* q = (struct fqueue*)arg;
    int i;
    void* data;
    for (i=0; i<100; i++)
    {
        data = fqueue_pop(q);
        while (data == NULL)
        {
            dprint("i = %d data = NULL", i);
            data = fqueue_pop(q);
        }

        dprint("i = %d data = %p", i, data);
        usleep(100);
    }
}

int main(int argc, char const *argv[])
{
    struct fqueue *q = fqueue_create(NULL);

    fqueue_push(q, (void*)(intptr_t)1);
    fqueue_push(q, (void*)(intptr_t)2);
    fqueue_push(q, (void*)(intptr_t)3);
    fqueue_push(q, (void*)(intptr_t)4);
    fqueue_push(q, (void*)(intptr_t)5);
    fqueue_push(q, (void*)(intptr_t)6);

    // dprint("fqueue_num = %d", fqueue_num(q));

    void* data;
    FQUEUE_FOREACH(q, data)
    {
        dprint("data = %p", data);
    }
    fqueue_release(q);

    q = fqueue_create(_cleanFn);

    fqueue_push(q, (void*)(intptr_t)10);
    fqueue_push(q, (void*)(intptr_t)20);
    fqueue_push(q, (void*)(intptr_t)30);
    int chk = fqueue_push(q, (void*)(intptr_t)40);
    dprint("chk = %d", chk);

    // dprint("fqueue_num = %d", fqueue_num(q));

    FQUEUE_FOREACH(q, data)
    {
        dprint("data = %p", data);
    }

    fqueue_release(q);

    q = fqueue_create(NULL);

    struct thread t[2] = {
        {_producer, q},
        {_consumer, q}
    };

    thread_join(t, 2);

    fqueue_release(q);

    dprint("ok");
    return 0;
}
