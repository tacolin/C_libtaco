#include <signal.h>

#include "basic.h"
#include "events.h"

static struct evloop _loop = {};
static struct ev _tm[3] = {0};

static void _process_timeout(struct evloop* loop, struct ev* ev, void* arg)
{
    dtrace();
    dprint("arg = %p, timeout\n", arg);
}

static void _process_sigint(struct evloop* loop, struct ev* ev, void* arg)
{
    dtrace();
    evloop_break(&_loop);
}

static void _process_ev(struct evloop* loop, struct ev* ev, void* arg)
{
    dprint("arg = %p", arg);
}

static void _process_stdin(struct evloop* loop, struct ev* ev, void* arg)
{
    char buf[256] = {0};
    int  readlen = read(ev->fd, buf, 256);
    CHECK_IF(readlen <= 0, return, "read failed");

    buf[readlen-1] = (buf[readlen-1] == '\n') ? '\0' : buf[readlen-1];

    if (strcmp(buf, "exit") == 0)
    {
        evloop_break(loop);
    }
    else if (strcmp(buf, "start tm1") == 0)
    {
        evtm_start(loop, &_tm[0]);
        dtrace();
    }
    else if (strcmp(buf, "start tm2") == 0)
    {
        evtm_start(loop, &_tm[1]);
        dtrace();
    }
    else if (strcmp(buf, "start tm3") == 0)
    {
        evtm_start(loop, &_tm[2]);
        dtrace();
    }
    else if (strcmp(buf, "stop tm1") == 0)
    {
        evtm_stop(loop, &_tm[0]);
        dtrace();
    }
    else if (strcmp(buf, "stop tm2") == 0)
    {
        evtm_stop(loop, &_tm[1]);
        dtrace();
    }
    else if (strcmp(buf, "stop tm3") == 0)
    {
        evtm_stop(loop, &_tm[2]);
        dtrace();
    }
    else if (strcmp(buf, "ev once") == 0)
    {
        int i;
        for (i=0; i<1000; i++)
        {
            ev_send(loop, _process_ev, (void*)(intptr_t)(i+1));
        }
    }
    else
    {
        dprint("unkown : %s", buf);
    }
    return;
}

int main(int argc, char const *argv[])
{
    struct ev io       = {};
    struct ev sig      = {};

    evloop_init(&_loop, 1000);

    evio_init(&io, 0, _process_stdin, NULL);
    evio_start(&_loop, &io);

    evsig_init(&sig, SIGINT, _process_sigint, NULL);
    evsig_start(&_loop, &sig);

    evtm_init(&_tm[0], 5000, 0, _process_timeout, (void*)1);
    evtm_init(&_tm[1], 2000, 2000, _process_timeout, (void*)2);
    evtm_init(&_tm[2], 10000, 0, _process_timeout, (void*)3);

    evloop_run(&_loop);

    evloop_uninit(&_loop);
    dprint("ok");
    return 0;
}
