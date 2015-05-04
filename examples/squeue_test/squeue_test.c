#include <stdint.h>

#include "basic.h"
#include "simple_thread.h"
#include "simple_queue.h"

#define MAX_ROUND 1000

tSqueue _queue;

static void _consumer(void* arg)
{
    tSqueue* queue = (tSqueue*)arg;

    int i;
    for (i=0; i<MAX_ROUND; i++)
    {
        void* ptr = NULL;
        while (ptr == NULL)
        {
            ptr = squeue_pop(queue);
            usleep(50);
        }

        dprint("pop %d, ptr = %p", i, ptr);
    }
}

static void _producer(void* arg)
{
    tSqueue* queue = (tSqueue*)arg;

    int i;
    for (i=0; i<MAX_ROUND; i++)
    {
        squeue_push(queue, (void*)((intptr_t)i+1));
        dprint("squeue push %d content = %p", i, (void*)((intptr_t)i+1));
        usleep(100);
    }
}

int main(int argc, char const *argv[])
{
    squeue_init(&_queue, NULL);

    tSthread t[2] = {
        {_producer, &_queue},
        {_consumer, &_queue},
    };

    sthread_join(t, 2);

    squeue_clean(&_queue);

    dprint("ok");

    return 0;
}
