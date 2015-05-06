#include "basic.h"
#include "ringbuf.h"

struct taco
{
    int a;
};

int main(int argc, char const *argv[])
{
    struct ringbuf rb = {};

    dprint("1st Ring Use Case : struct taco Structure write and read");

    ringbuf_init(&rb, 10, sizeof(struct taco), NULL);

    int i;
    struct taco taco;
    for (i=0; i<115; i++)
    {
        taco.a = i;
        ringbuf_write(&rb, &taco);
    }
    dprint("ringbuf is %s", ringbuf_full(&rb) ? "full" : "not full");

    while (!ringbuf_empty(&rb))
    {
        ringbuf_read(&rb, &taco);
        dprint("taco.a = %d", taco.a);
    }
    dprint("ringbuf is %s", ringbuf_empty(&rb) ? "empty" : "not empty");

    ringbuf_uninit(&rb);

    dprint("");

    dprint("2nd Ring Use Case : struct taco Structure pre-write and pre-read");

    void* data = calloc(sizeof(struct taco), 10);
    ringbuf_init(&rb, 10, sizeof(struct taco), data);

    struct taco *pre;
    for (i=0; i<47; i++)
    {
        pre = (struct taco*)ringbuf_pre_write(&rb);
        pre->a = i;
        ringbuf_post_write(&rb);
    }
    dprint("ringbuf is %s", ringbuf_full(&rb) ? "full" : "not full");

    while (pre = ringbuf_pre_read(&rb))
    {
        dprint("taco.a = %d", pre->a);
        ringbuf_post_read(&rb);
    }
    dprint("ringbuf is %s", ringbuf_empty(&rb) ? "empty" : "not empty");

    ringbuf_uninit(&rb);
    free(data);

    dprint("");

    dprint("3nd Ring Use Case : struct taco Structure get-tail and get-prev");

    ringbuf_init(&rb, 10, sizeof(struct taco), NULL);

    for (i=0 ; i<10 ; i++)
    {
        taco.a = i;
        ringbuf_write(&rb, &taco);
    }
    dprint("ringbuf is %s", ringbuf_full(&rb) ? "full" : "not full");

    pre = ringbuf_tail(&rb);
    for (i=0 ; i<15 ; i++)
    {
        dprint("taco.a = %d", pre->a);
        pre = ringbuf_prev(&rb, pre);
    }

    ringbuf_uninit(&rb);

    dprint("over");
    return 0;
}
