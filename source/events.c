#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/epoll.h>

#include "events.h"

#define EPOLL_WAIT_MS     (500)

#define EV_ST_NULL        (1)
#define EV_ST_INIT        (2)
#define EV_ST_RUNNING     (3)
#define EV_ST_WAITING_END (4)

#define EV_ACT_START (1)
#define EV_ACT_STOP  (2)

#define EV_IO     (1)
#define EV_TIMER  (2)
#define EV_SIGNAL (3)
#define EV_PURE   (4)

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

struct evact
{
    int action;
    struct ev* ev;
};

static void _clean_act(void* input)
{
    if (input == NULL) return;

    struct evact* act = (struct evact*)input;
    if (act->ev && (act->ev->type == EV_PURE))
    {
        free(act->ev);
    }
    free(act);
}

int evloop_init(struct evloop* loop, int max_ev_num)
{
    CHECK_IF(loop == NULL, return EV_FAIL, "loop is null");
    CHECK_IF(max_ev_num <= 0, return EV_FAIL, "max_ev_num = %d invalid", max_ev_num);

    memset(loop, 0, sizeof(struct evloop));
    loop->act_queue = fqueue_create(_clean_act);
    loop->epfd      = epoll_create(max_ev_num);

    loop->qfd = eventfd(0, 0);
    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = &loop->qfd
    };
    epoll_ctl(loop->epfd, EPOLL_CTL_ADD, loop->qfd, &tmp);

    loop->max_ev_num = max_ev_num;
    loop->state      = EV_ST_INIT;
    return EV_OK;
}

void evloop_uninit(struct evloop* loop)
{
    CHECK_IF(loop == NULL, return, "loop is null");
    CHECK_IF(loop->state < EV_ST_INIT, return, "loop is not init yet");

    if (loop->state == EV_ST_RUNNING) evloop_break(loop);

    struct epoll_event tmp = {};
    epoll_ctl(loop->epfd, EPOLL_CTL_DEL, loop->qfd, &tmp);
    close(loop->qfd);
    loop->qfd = -1;

    fqueue_release(loop->act_queue);
    close(loop->epfd);
    loop->epfd       = -1;
    loop->max_ev_num = 0;
    loop->state      = EV_ST_NULL;
    return;
}

static void _handle_ev(struct evloop* loop, struct ev* ev)
{
    CHECK_IF(loop == NULL, return, "loop is null");
    CHECK_IF(loop->state < EV_ST_RUNNING, return, "loop is not running yet");
    CHECK_IF(ev == NULL, return, "ev is null");
    CHECK_IF(ev->fd < 0, return, "ev->fd = %d invalid", ev->fd);

    if (ev->type == EV_TIMER)
    {
        uint64_t val;
        ssize_t  sz = sizeof(val);
        read(ev->fd, &val, sz);

        if (ev->interval_ms <= 0)
        {
            struct epoll_event tmp = {};
            epoll_ctl(loop->epfd, EPOLL_CTL_DEL, ev->fd, &tmp);
            close(ev->fd);
            ev->fd = -1;
        }
    }
    else if (ev->type == EV_SIGNAL)
    {
        uint64_t val;
        ssize_t  sz = sizeof(val);
        read(ev->fd, &val, sz);
    }
    ev->callback(loop, ev, ev->arg);
}

static void _handle_acts(struct evloop* loop)
{
    CHECK_IF(loop == NULL, return, "loop is null");
    CHECK_IF(loop->state < EV_ST_RUNNING, return, "loop is not running yet");

    eventfd_t val;
    eventfd_read(loop->qfd, &val);

    struct evact* act;
    struct ev* ev;
    FQUEUE_FOREACH(loop->act_queue, act)
    {
        struct epoll_event tmp = {};
        ev = act->ev;
        if (act->action == EV_ACT_START)
        {
            if (ev->type == EV_PURE)
            {
                ev->callback(loop, ev, ev->arg); // execute callback directly
                free(ev);
            }
            else
            {
                if (ev->type == EV_TIMER)
                {
                    ev->fd = timerfd_create(CLOCK_MONOTONIC, 0);
                    struct itimerspec timeval = {
                        .it_value.tv_sec     = ev->time_ms / 1000,
                        .it_value.tv_nsec    = (ev->time_ms % 1000) * 1000 * 1000,
                        .it_interval.tv_sec  = ev->interval_ms / 1000,
                        .it_interval.tv_nsec = (ev->interval_ms % 1000) * 1000 * 1000
                    };
                    timerfd_settime(ev->fd, 0, &timeval, NULL);
                }
                else if (ev->type == EV_SIGNAL)
                {
                    sigset_t mask;
                    sigemptyset(&mask);
                    sigaddset(&mask, ev->signum);
                    sigprocmask(SIG_BLOCK, &mask, NULL);
                    ev->fd = signalfd(-1, &mask, 0);
                }

                tmp.events = EPOLLIN;
                tmp.data.ptr = ev;
                epoll_ctl(loop->epfd, EPOLL_CTL_ADD, ev->fd, &tmp);
            }
        }
        else if (act->action == EV_ACT_STOP)
        {
            epoll_ctl(loop->epfd, EPOLL_CTL_DEL, ev->fd, &tmp);

            if (ev->type == EV_TIMER)
            {
                struct itimerspec timeval = {};
                timerfd_settime(ev->fd, 0, &timeval, NULL);
                close(ev->fd);
                ev->fd = -1;
            }
            else if (ev->type == EV_SIGNAL)
            {
                sigset_t mask;
                sigemptyset(&mask);
                sigaddset(&mask, ev->signum);
                sigprocmask(SIG_UNBLOCK, &mask, NULL);
                close(ev->fd);
                ev->fd = -1;
            }
        }
        free(act);
    }
    return;
}

void evloop_run(struct evloop* loop)
{
    CHECK_IF(loop == NULL, return, "loop is null");
    CHECK_IF(loop->state < EV_ST_INIT, return, "loop is not init yet");
    CHECK_IF(loop->state >= EV_ST_RUNNING, return, "loop is already running");
    CHECK_IF(loop->epfd <= 0, return, "loop->epfd = %d invalid", loop->epfd);

    loop->tid = pthread_self();
    struct epoll_event evbuf[loop->max_ev_num];
    memset(evbuf, 0, sizeof(struct epoll_event) * loop->max_ev_num);

    loop->state = EV_ST_RUNNING;

    int i, ev_num;
    struct ev* ev;
    struct evact* act;
    while (loop->state == EV_ST_RUNNING)
    {
        ev_num = epoll_wait(loop->epfd, evbuf, loop->max_ev_num, EPOLL_WAIT_MS);
        CHECK_IF(ev_num < 0, break, "epoll_wait fialed");

        if (ev_num > 0)
        {
            for (i=0; i<ev_num && (loop->state == EV_ST_RUNNING); i++)
            {
                ev = (struct ev*)(evbuf[i].data.ptr);
                if (ev)
                {
                    if ((void*)ev == &loop->qfd)
                    {
                        _handle_acts(loop);
                    }
                    else
                    {
                        _handle_ev(loop, ev);
                    }
                }
            }
        }
    }
    loop->state = EV_ST_INIT;
    loop->tid   = 0;
    return;
}

void evloop_break(struct evloop* loop)
{
    CHECK_IF(loop == NULL, return, "loop is null");

    if (loop->state < EV_ST_RUNNING) return;

    loop->state = EV_ST_WAITING_END;
    if (pthread_self() != loop->tid)
    {
        int i;
        for (i=0; (i<10) && (loop->state > EV_ST_INIT); i++)
        {
            struct timeval timeout = {.tv_usec = 5000};
            select(0, 0, 0, 0, &timeout);
        }
    }
    return;
}

int evio_init(struct ev* ev, int fd, void (*callback)(struct evloop*, struct ev*, void*), void* arg)
{
    CHECK_IF(ev == NULL, return EV_FAIL, "ev is null");
    CHECK_IF(fd < 0, return EV_FAIL, "fd = %d invalid", fd);
    CHECK_IF(callback == NULL, return EV_FAIL, "callback is null");

    memset(ev, 0, sizeof(struct ev));
    ev->fd       = fd;
    ev->type     = EV_IO;
    ev->arg      = arg;
    ev->callback = callback;
    return EV_OK;
}

static int _start_ev(struct evloop* loop, struct ev* ev)
{
    CHECK_IF(loop == NULL, return EV_FAIL, "loop is null");
    CHECK_IF(loop->state < EV_ST_INIT, return EV_FAIL, "loop is not init yet");
    CHECK_IF(loop->qfd < 0, return EV_FAIL, "loop->qfd = %d invalid", loop->qfd);
    CHECK_IF(ev == NULL, return EV_FAIL, "ev is null");
    // CHECK_IF(ev->fd < 0, return EV_FAIL, "ev->fd = %d invalid", ev->fd);

    struct evact* act = calloc(sizeof(struct evact), 1);
    act->ev     = ev;
    act->action = EV_ACT_START;

    fqueue_push(loop->act_queue, act);

    eventfd_t val = loop->qfd;
    eventfd_write(loop->qfd, val);
    return EV_OK;
}

static _stop_ev(struct evloop* loop, struct ev* ev)
{
    CHECK_IF(loop == NULL, return, "loop is null");
    CHECK_IF(loop->state < EV_ST_INIT, return, "loop is not init yet");
    CHECK_IF(loop->qfd < 0, return, "loop->qfd = %d invalid", loop->qfd);
    CHECK_IF(ev == NULL, return, "ev is null");
    CHECK_IF(ev->fd < 0, return, "ev->fd = %d invalid", ev->fd);

    struct evact* act = calloc(sizeof(struct evact), 1);
    act->ev     = ev;
    act->action = EV_ACT_STOP;

    fqueue_push(loop->act_queue, act);

    eventfd_t val = 1;
    eventfd_write(loop->qfd, val);
    return;
}

int evio_start(struct evloop* loop, struct ev* ev)
{
    return _start_ev(loop, ev);
}

void evio_stop(struct evloop* loop, struct ev* ev)
{
    _stop_ev(loop, ev);
}

int evsig_init(struct ev* ev, int signum, void (*callback)(struct evloop*, struct ev*, void*), void* arg)
{
    CHECK_IF(ev == NULL, return EV_FAIL, "ev is null");
    CHECK_IF(callback == NULL, return EV_FAIL, "callback is null");

    memset(ev, 0, sizeof(struct ev));
    ev->fd       = -1;
    ev->type     = EV_SIGNAL;
    ev->arg      = arg;
    ev->callback = callback;
    ev->signum   = signum;
    return EV_OK;
}

int evsig_start(struct evloop* loop, struct ev* ev)
{
    return _start_ev(loop, ev);
}

void evsig_stop(struct evloop* loop, struct ev* ev)
{
    _stop_ev(loop, ev);
}

int evtm_init(struct ev* ev, int time_ms, int interval_ms, void (*callback)(struct evloop*, struct ev*, void*), void* arg)
{
    CHECK_IF(ev == NULL, return EV_FAIL, "ev is null");
    CHECK_IF((time_ms == 0) && (interval_ms == 0), return EV_FAIL, "time_ms = interval_ms = 0");
    CHECK_IF(callback == NULL, return EV_FAIL, "callback is null");

    memset(ev, 0, sizeof(struct ev));
    ev->fd          = -1;
    ev->type        = EV_TIMER;
    ev->arg         = arg;
    ev->callback    = callback;
    ev->time_ms     = time_ms;
    ev->interval_ms = interval_ms;
    return EV_OK;
}

int evtm_start(struct evloop* loop, struct ev* ev)
{
    return _start_ev(loop, ev);
}

void evtm_stop(struct evloop* loop, struct ev* ev)
{
    _stop_ev(loop, ev);
}

int ev_send(struct evloop* loop, void (*callback)(struct evloop*, struct ev*, void*), void* arg)
{
    CHECK_IF(loop == NULL, return EV_FAIL, "loop is null");
    CHECK_IF(callback == NULL, return EV_FAIL, "callback is null");

    struct ev* ev = calloc(sizeof(struct ev), 1);
    ev->fd          = -1;
    ev->type        = EV_PURE;
    ev->arg         = arg;
    ev->callback    = callback;

    int chk = _start_ev(loop, ev);
    if (chk != EV_OK) free(ev);

    return chk;
}
