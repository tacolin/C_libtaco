#include "basic.h"
#include "oldevents.h"
#include "task.h"

#include <signal.h>

static tTask _taskNew;
static tOevLoop _loopNew;

static tOevTimer _timer1;
static tOevTimer _timer2;

static void _runLoopNew(void* task, void* arg)
{
    tOevLoop* loop = (tOevLoop*)arg;

    oevloop_init(loop);

    oevloop_run(loop);

    oevloop_uninit(loop);

    return;
}

static int _timer1Expired(tOevLoop* loop, void* ev, void* arg)
{
    dtrace();
    dprint("_timer1Expired");
    return 0;
}

static int _timer2Expired(tOevLoop* loop, void* ev, void* arg)
{
    dtrace();
    dprint("_timer2Expired");
    return 0;
}

static int _onceCallback(tOevLoop* loop, void* ev, void* arg)
{
    dtrace();
    long i = (long)arg;
    dprint("i = %ld", i);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

static int _processStdin(tOevLoop* loop, void* ev, void* arg)
{
    tOevIo* io = (tOevIo*)ev;

    char buf[256] = {0};
    int  readlen = read(io->fd, buf, 256);
    check_if(readlen <= 0, return -1, "read failed");

    buf[readlen-1] = (buf[readlen-1] == '\n') ? '\0' : buf[readlen-1];

    if (strcmp(buf, "exit") == 0)
    {
        oevloop_break(loop);
    }
    else if (strcmp(buf, "start new loop") == 0)
    {
        task_init(&_taskNew, "TASK NEW", _runLoopNew, &_loopNew, TASK_NORMAL, TASK_ONESHOT);
        task_start(&_taskNew);
    }
    else if (strcmp(buf, "stop new loop") == 0)
    {
        task_stop(&_taskNew);
    }
    else if (strcmp(buf, "break new loop") == 0)
    {
        oevloop_break(&_loopNew);
    }
    else if (strcmp(buf, "start timer1") == 0)
    {
        oevtm_init(&_timer1, _timer1Expired, 5000, NULL, OEV_TIMER_ONESHOT);
        dtrace();
        oevtm_start(&_loopNew, &_timer1);
    }
    else if (strcmp(buf, "stop timer1") == 0)
    {
        dtrace();
        oevtm_stop(&_loopNew, &_timer1);
    }
    else if (strcmp(buf, "start timer2") == 0)
    {
        oevtm_init(&_timer2, _timer2Expired, 2000, NULL, OEV_TIMER_PERIODIC);
        dtrace();
        oevtm_start(&_loopNew, &_timer2);
    }
    else if (strcmp(buf, "stop timer2") == 0)
    {
        dtrace();
        oevtm_stop(&_loopNew, &_timer2);
    }
    else if (strcmp(buf, "evonce") == 0)
    {
        long i = 0;
        for (i=0; i<10; i++)
        {
            dtrace();
            oev_once(&_loopNew, _onceCallback, (void*)i);
        }
    }
    else
    {
        dprint("[taco] %s", buf);
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    tOevIo io;
    tOevLoop loop;

    task_system_init();

    dtrace();

    oevloop_init(&loop);

    oevio_init(&io, 0, _processStdin, NULL);
    oevio_start(&loop, &io);

    oevloop_run(&loop);

    oevloop_uninit(&loop);

    task_system_uninit();

    dprint("ok");
    return 0;
}
