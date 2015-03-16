#include "basic.h"
#include "ringbuf.h"

typedef struct tTaco
{
    int a;

} tTaco;

int main(int argc, char const *argv[])
{
    tRingBuf rb = {};

    dprint("1st Ring Use Case : tTaco Structure write and read");

    ringbuf_init(&rb, 10, sizeof(tTaco), NULL);

    int i;
    tTaco taco;
    for (i=0; i<115; i++)
    {
        taco.a = i;
        ringbuf_write(&rb, &taco);
    }
    dprint("ringbuf is %s", ringbuf_isFull(&rb) ? "full" : "not full");

    while (!ringbuf_isEmpty(&rb))
    {
        ringbuf_read(&rb, &taco);
        dprint("taco.a = %d", taco.a);
    }
    dprint("ringbuf is %s", ringbuf_isEmpty(&rb) ? "empty" : "not empty");

    ringbuf_uninit(&rb);

    dprint("");

    dprint("2nd Ring Use Case : tTaco Structure pre-write and pre-read");

    void* data = calloc(sizeof(tTaco), 10);
    ringbuf_init(&rb, 10, sizeof(tTaco), data);

    tTaco *pre;
    for (i=0; i<47; i++)
    {
        pre = (tTaco*)ringbuf_preWrite(&rb);
        pre->a = i;
        ringbuf_postWrite(&rb);
    }
    dprint("ringbuf is %s", ringbuf_isFull(&rb) ? "full" : "not full");

    while (pre = ringbuf_preRead(&rb))
    {
        dprint("taco.a = %d", pre->a);
        ringbuf_postRead(&rb);
    }
    dprint("ringbuf is %s", ringbuf_isEmpty(&rb) ? "empty" : "not empty");

    ringbuf_uninit(&rb);
    free(data);

    dprint("");

    dprint("3nd Ring Use Case : tTaco Structure get-tail and get-prev");

    ringbuf_init(&rb, 10, sizeof(tTaco), NULL);

    for (i=0 ; i<10 ; i++)
    {
        taco.a = i;
        ringbuf_write(&rb, &taco);
    }
    dprint("ringbuf is %s", ringbuf_isFull(&rb) ? "full" : "not full");

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
