#include "basic.h"
#include "oldevents.h"
#include <string.h>

static int _running = 1;

////////////////////////////////////////////////////////////////////////////////

static void _processStdin(int fd)
{
    char buf[256] = {0};
    int  readlen = read(fd, buf, 256);
    check_if(readlen <= 0, return, "read failed");

    buf[readlen-1] = (buf[readlen-1] == '\n') ? '\0' : buf[readlen-1];

    if (strcmp(buf, "exit") == 0)
    {
        _running = 0;
    }
    // else if (strcmp(buf, "start new loop") == 0)
    // {
    //     task_init(&_taskNew, "TASK NEW", _runLoopNew, &_loopNew, TASK_NORMAL, TASK_ONESHOT);
    //     task_start(&_taskNew);
    // }
    // else if (strcmp(buf, "stop new loop") == 0)
    // {
    //     evloop_break(&_loopNew);
    //     task_stop(&_taskNew);
    // }
    // else if (strcmp(buf, "break new loop") == 0)
    // {
    //     evloop_break(&_loopNew);
    // }
    // else if (strcmp(buf, "start timer1") == 0)
    // {
    //     evtm_init(&_timer1, _timer1Expired, 5000, NULL, EV_TIMER_ONESHOT);
    //     dtrace();
    //     evtm_start(&_loopNew, &_timer1);
    // }
    // else if (strcmp(buf, "stop timer1") == 0)
    // {
    //     dtrace();
    //     evtm_stop(&_loopNew, &_timer1);
    // }
    // else if (strcmp(buf, "start timer2") == 0)
    // {
    //     evtm_init(&_timer2, _timer2Expired, 2000, NULL, EV_TIMER_PERIODIC);
    //     dtrace();
    //     evtm_start(&_loopNew, &_timer2);
    // }
    // else if (strcmp(buf, "stop timer2") == 0)
    // {
    //     dtrace();
    //     evtm_stop(&_loopNew, &_timer2);
    // }
    // else if (strcmp(buf, "pause timer2") == 0)
    // {
    //     dtrace();
    //     evtm_pause(&_loopNew, &_timer2);
    // }
    // else if (strcmp(buf, "resume timer2") == 0)
    // {
    //     dtrace();
    //     evtm_resume(&_loopNew, &_timer2);
    // }
    // else if (strcmp(buf, "start timer3") == 0)
    // {
    //     evtm_init(&_timer3, _timer3Expired, 10000, NULL, EV_TIMER_ONESHOT);
    //     dtrace();
    //     evtm_start(&_loopNew, &_timer3);
    // }
    // else if (strcmp(buf, "pause timer3") == 0)
    // {
    //     dtrace();
    //     evtm_pause(&_loopNew, &_timer3);
    // }
    // else if (strcmp(buf, "resume timer3") == 0)
    // {
    //     dtrace();
    //     evtm_resume(&_loopNew, &_timer3);
    // }
    // else if (strcmp(buf, "stop timer3") == 0)
    // {
    //     dtrace();
    //     evtm_stop(&_loopNew, &_timer3);
    // }
    // else if (strcmp(buf, "evonce") == 0)
    // {
    //     dtrace();
    //     long i = 0;
    //     for (i=0; i<10; i++)
    //     {
    //         ev_once(&_loopNew, _onceCallback, (void*)i);
    //     }
    // }
    else
    {
        dprint("[taco] %s", buf);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    fpoll_init();
    tmfd_init();

    int fpd = fpoll_create(100);

    struct fpoll_event ev = {
        .events = FPOLLIN,
        .data.fd = 0,
    };
    fpoll_ctl(fpd, FPOLL_CTL_ADD, 0, &ev);

    int tm1 = tmfd_create(TM_CLOCK_MONOTONIC, 0);

    memset(&ev, 0, sizeof(struct fpoll_event));
    ev.events = FPOLLIN;
    ev.data.fd = tm1;
    fpoll_ctl(fpd, FPOLL_CTL_ADD, tm1, &ev);

    struct itmspec timeval = {
        .it_value.tv_sec = 5,
    };
    tmfd_settime(tm1, 0, &timeval, NULL);
    dprint("tm1 start");
    dtrace();

    int tm2 = tmfd_create(TM_CLOCK_MONOTONIC, 0);
    memset(&ev, 0, sizeof(struct fpoll_event));
    ev.events = FPOLLIN;
    ev.data.fd = tm2;
    fpoll_ctl(fpd, FPOLL_CTL_ADD, tm2, &ev);

    memset(&timeval, 0, sizeof(struct itmspec));
    timeval.it_value.tv_nsec = 500 * 1000 * 1000;
    timeval.it_interval.tv_nsec = 500 * 1000 * 1000;
    tmfd_settime(tm2, 0, &timeval, NULL);
    dprint("tm2 start");
    dtrace();

    int tm2_count = 0;

    struct fpoll_event evbuf[10] = {0};

    while (_running)
    {
        int num = fpoll_wait(fpd, evbuf, 10, 10);
        int i;
        for (i=0; i<num; i++)
        {
            if (evbuf[i].data.fd == 0)
            {
                _processStdin(evbuf[i].data.fd);
            }
            else if (evbuf[i].data.fd == tm1)
            {
                uint64_t dummy;
                ssize_t dummy_size = sizeof(dummy);
                read(tm1, &dummy, dummy_size);
                dprint("tm1 timeout");
                dtrace();
            }
            else if (evbuf[i].data.fd == tm2)
            {
                uint64_t dummy;
                ssize_t dummy_size = sizeof(dummy);
                read(tm2, &dummy, dummy_size);
                dprint("tm2 timeout");
                dtrace();

                tm2_count++;
                if (tm2_count >= 10)
                {
                    struct itmspec tmp = {};
                    tmfd_settime(tm2, 0, &tmp, NULL);
                    dprint("stop tm2");
                    tm2_count = 0;
                }
            }
        }
    }

    fpoll_close(fpd);

    tmfd_uninit();
    fpoll_uninit();

    dprint("ok");
    return 0;
}
