#include "basic.h"
#include "fpoll.h"
#include "tmfd.h"

static int _running = 1;

static int _tm1;
static int _tm2;

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
    else if (strcmp(buf, "start tm1") == 0)
    {
        struct itmfdspec timeval = {};

        timeval.it_value.tv_sec = 5;
        tmfd_settime(_tm1, 0, &timeval, NULL);
        dtrace();
    }
    else if (strcmp(buf, "start tm2") == 0)
    {
        struct itmfdspec timeval = {};

        timeval.it_value.tv_nsec = 500 * 1000 * 1000;
        timeval.it_interval.tv_nsec = 500 * 1000 * 1000;
        
        tmfd_settime(_tm2, 0, &timeval, NULL);
        dtrace();
    }
    else
    {
        dprint("[taco] %s", buf);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    fpoll_system_init();
    tmfd_system_init();

    int fpd = fpoll_create(100);

    struct fpoll_event ev = {
        .events = FPOLLIN,
        .data.fd = 0,
    };

    fpoll_ctl(fpd, FPOLL_CTL_ADD, 0, &ev);

    _tm1 = tmfd_create(TMFD_CLOCK_MONOTONIC, 0);

    memset(&ev, 0, sizeof(struct fpoll_event));
    ev.events = FPOLLIN;
    ev.data.fd = _tm1;
    fpoll_ctl(fpd, FPOLL_CTL_ADD, _tm1, &ev);

    struct itmfdspec timeval = {
        .it_value.tv_sec = 5,
    };
    tmfd_settime(_tm1, 0, &timeval, NULL);
    dprint("_tm1 start");
    dtrace();

    _tm2 = tmfd_create(TMFD_CLOCK_MONOTONIC, 0);
    memset(&ev, 0, sizeof(struct fpoll_event));
    ev.events = FPOLLIN;
    ev.data.fd = _tm2;
    fpoll_ctl(fpd, FPOLL_CTL_ADD, _tm2, &ev);

    memset(&timeval, 0, sizeof(struct itmfdspec));
    timeval.it_value.tv_nsec = 500 * 1000 * 1000;
    timeval.it_interval.tv_nsec = 500 * 1000 * 1000;
    tmfd_settime(_tm2, 0, &timeval, NULL);
    dprint("_tm2 start");
    dtrace();

    int tm2_count = 0;

    struct fpoll_event evbuf[10] = {0};

    int event_tm[10];
    int j;

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
            else if (evbuf[i].data.fd == _tm1)
            {
                uint64_t dummy;
                ssize_t dummy_size = sizeof(dummy);
                read(_tm1, &dummy, dummy_size);
                dprint("_tm1 timeout");
                dtrace();
            }
            else if (evbuf[i].data.fd == _tm2)
            {
                uint64_t dummy;
                ssize_t dummy_size = sizeof(dummy);
                read(_tm2, &dummy, dummy_size);
                dprint("_tm2 timeout");
                dtrace();

                tm2_count++;
                if (tm2_count >= 10)
                {
                    struct itmfdspec tmp = {};
                    tmfd_settime(_tm2, 0, &tmp, NULL);
                    dprint("stop _tm2");
                    tm2_count = 0;

                    for (j=0; j<10; j++)
                    {
                        event_tm[j] = tmfd_create(TMFD_CLOCK_MONOTONIC, 0);

                        struct fpoll_event local_ev = {
                            .events = FPOLLIN,
                            .data.fd = event_tm[j],
                        };

                        fpoll_ctl(fpd, FPOLL_CTL_ADD, event_tm[j], &local_ev);

                        tmp.it_value.tv_nsec = 100;
                        tmfd_settime(event_tm[j], 0, &tmp, NULL);

                        dprint("create fd = %d", event_tm[j]);
                        dtrace();
                    }
                }
            }
            else
            {
                uint64_t dummy;
                ssize_t dummy_size = sizeof(dummy);
                read(evbuf[i].data.fd, &dummy, dummy_size);
                dprint("other fd = %d", evbuf[i].data.fd);
                dtrace();

                struct fpoll_event ev = {};
                fpoll_ctl(fpd, FPOLL_CTL_DEL, evbuf[i].data.fd, &ev);

                tmfd_close(evbuf[i].data.fd); 
            }
        }
    }

    fpoll_close(fpd);

    tmfd_system_uninit();
    fpoll_system_uninit();

    dprint("ok");
    return 0;
}
