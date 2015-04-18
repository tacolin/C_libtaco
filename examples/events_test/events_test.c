#include "basic.h"
#include "events.h"
#include "task.h"

static tTask _taskNew;
static tEvLoop _loopNew;

static tEvTimer _timer1;
static tEvTimer _timer2;

static void _runLoopNew(void* task, void* arg)
{
    tEvLoop* loop = (tEvLoop*)arg;

    evloop_init(loop, 1000);

    evloop_run(loop);

    evloop_uninit(loop);

    return;
}

static void _timer1Expired(tEvLoop* loop, tEv* ev, void* arg)
{
    dtrace();
    dprint("_timer1Expired");
    return;
}

static void _timer2Expired(tEvLoop* loop, tEv* ev, void* arg)
{
    dtrace();
    dprint("_timer2Expired");
    return;
}

static void _onceCallback(tEvLoop* loop, tEv* ev, void* arg)
{
    dtrace();
    long i = (long)arg;
    dprint("i = %ld", i);
    return;
}

////////////////////////////////////////////////////////////////////////////////

static void _processStdin(tEvLoop* loop, tEv* ev, void* arg)
{
    tEvIo* io = (tEvIo*)ev;

    char buf[256] = {0};
    int  readlen = read(io->fd, buf, 256);
    check_if(readlen <= 0, return, "read failed");

    buf[readlen-1] = (buf[readlen-1] == '\n') ? '\0' : buf[readlen-1];

    if (strcmp(buf, "exit") == 0)
    {
        evloop_break(loop);
    }
    else if (strcmp(buf, "start new loop") == 0)
    {
        task_init(&_taskNew, "TASK NEW", _runLoopNew, &_loopNew, TASK_NORMAL, TASK_ONESHOT);
        task_start(&_taskNew);
    }
    else if (strcmp(buf, "stop new loop") == 0)
    {
        // evloop_break(&_loopNew);
        task_stop(&_taskNew);
    }
    else if (strcmp(buf, "break new loop") == 0)
    {
        evloop_break(&_loopNew);
    }
    else if (strcmp(buf, "start timer1") == 0)
    {
        evtm_init(&_timer1, _timer1Expired, 5000, NULL, EV_TIMER_ONESHOT);
        dtrace();
        evtm_start(&_loopNew, &_timer1);
    }
    else if (strcmp(buf, "stop timer1") == 0)
    {
        dtrace();
        evtm_stop(&_loopNew, &_timer1);
    }
    else if (strcmp(buf, "start timer2") == 0)
    {
        evtm_init(&_timer2, _timer2Expired, 2000, NULL, EV_TIMER_PERIODIC);
        dtrace();
        evtm_start(&_loopNew, &_timer2);
    }
    else if (strcmp(buf, "stop timer2") == 0)
    {
        dtrace();
        evtm_stop(&_loopNew, &_timer2);
    }
    else if (strcmp(buf, "evonce") == 0)
    {
        long i = 0;
        for (i=0; i<10; i++)
        {
            dtrace();
            ev_once(&_loopNew, _onceCallback, (void*)i);
        }
    }
    else
    {
        dprint("[taco] %s", buf);
    }

    return;
}

int main(int argc, char const *argv[])
{
    tEvIo io;
    tEvLoop loop;

    task_system_init();

    dtrace();

    evloop_init(&loop, 1000);

    evio_init(&io, _processStdin, 0, NULL);
    evio_start(&loop, &io);

    evloop_run(&loop);

    evloop_uninit(&loop);

    task_system_uninit();

    dprint("ok");
    return 0;
}
