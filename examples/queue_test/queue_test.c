#include "basic.h"
#include "queue.h"

int main(int argc, char const *argv[])
{
    tQueue queue;

    queue_init(&queue, 5, NULL, QUEUE_UNSUSPEND, QUEUE_UNSUSPEND);

    queue_push(&queue, (void*)1);
    queue_push(&queue, (void*)2);
    queue_push(&queue, (void*)3);
    queue_push(&queue, (void*)4);
    queue_push(&queue, (void*)5);
    queue_push(&queue, (void*)6);

    dprint("show 1, 2, 3, 4, 5");

    long* get = NULL;
    for (get = queue_pop(&queue); get != NULL; get = queue_pop(&queue))
    {
        dprint("get = %ld", (long)get);
    }

    long a = 10;
    long b = 20;
    long c = 30;

    queue_push(&queue, &c);
    queue_push(&queue, &b);
    queue_push(&queue, &a);

    dprint("show 30, 20, 10");
    for (get = queue_pop(&queue); get != NULL; get = queue_pop(&queue))
    {
        dprint("get = %ld", *get);
    }

    queue_clean(&queue);

    dprint("ok");

    return 0;
}
