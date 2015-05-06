#ifndef _EVENTS_H_
#define _EVENTS_H_

#include <pthread.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>

#include "fast_queue.h"

#define EV_OK (0)
#define EV_FAIL (-1)

struct evloop
{
    int epfd;
    int max_ev_num;
    int state;

    int qfd;
    struct fqueue* act_queue;

    pthread_t tid;
};

struct ev
{
    int fd;
    int type;

    void* arg;
    void (*callback)(struct evloop* loop, struct ev* ev, void* arg);

    union
    {
        int signum;
        struct
        {
            int time_ms;
            int interval_ms;
        };
    };
};

int evloop_init(struct evloop* loop, int max_ev_num);
void evloop_uninit(struct evloop* loop);

void evloop_run(struct evloop* loop);
void evloop_break(struct evloop* loop);

int evio_init(struct ev* ev, int fd, void (*callback)(struct evloop*, struct ev*, void*), void* arg);
int evio_start(struct evloop* loop, struct ev* ev);
void evio_stop(struct evloop* loop, struct ev* ev);

int evsig_init(struct ev* ev, int signum, void (*callback)(struct evloop*, struct ev*, void*), void* arg);
int evsig_start(struct evloop* loop, struct ev* ev);
void evsig_stop(struct evloop* loop, struct ev* ev);

int evtm_init(struct ev* ev, int time_ms, int interval_ms, void (*callback)(struct evloop*, struct ev*, void*), void* arg);
int evtm_start(struct evloop* loop, struct ev* ev);
void evtm_stop(struct evloop* loop, struct ev* ev);

int ev_send(struct evloop* loop, void (*callback)(struct evloop*, struct ev*, void*), void* arg);

#endif //_EVENTS_H_
