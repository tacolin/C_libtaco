#include "basic.h"
#include "events.h"

static void _sigIntCb(tEvLoop* loop, tEvSignal* sig, void* arg)
{
    dprint("SIG INT CB called - break loop");
    evloop_break(loop);
}

static void _stdinCb(tEvLoop* loop, tEvIo* io, void* arg)
{
    static char buf[256] = {0};
    int readlen = read(io->fd, buf, 256);
    check_if(readlen <= 0, return, "readlen = %d", readlen);
    buf[readlen] = '\0';

    dprint("read from stdin : %s", buf);
    return;
}

static void _onceCb(tEvLoop* loop, void* arg)
{
    dprint("recv ev once, data = %ld", (long)arg);
}

static void _tm1Cb(tEvLoop* loop, tEvTimer* tm, void* arg)
{
    dprint("TIMEOUT 1 - send 100 ev once to loop");
    long i;
    for (i=0; i<100; i++)
    {
        ev_once(loop, _onceCb, (void*)i);
    }
}

static void _tm2Cb(tEvLoop* loop, tEvTimer* tm, void* arg)
{
    static int count = 0;
    dprint("TIMEOUT 2 - periodically, count = %d", count);
    count++;
}

int main(int argc, char const *argv[])
{
    tEvLoop loop = {};
    evloop_init(&loop, 100);

    tEvSignal sig = {};
    evsig_init(&sig, _sigIntCb, SIGINT, NULL);
    evsig_start(&loop, &sig);

    tEvIo io = {};
    evio_init(&io, _stdinCb, 0 /* STDIN */, NULL);
    evio_start(&loop, &io);

    tEvTimer tm1 = {};
    evtm_init(&tm1, _tm1Cb, 5000, NULL, EV_TIMER_ONSHOT);
    evtm_start(&loop, &tm1);

    tEvTimer tm2 = {};
    evtm_init(&tm2, _tm2Cb, 1000, NULL, EV_TIMER_PERIODIC);
    evtm_start(&loop, &tm2);

    evloop_run(&loop);

    evloop_uninit(&loop);
    dprint("over");
    return 0;
}
