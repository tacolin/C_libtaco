#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "basic.h"
#include "list.h"
#include "lock.h"

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>
#include <signal.h>

typedef enum 
{
    EV_OK = 0,
    EV_ERROR = -1

} tEvStatus;

typedef struct tEvLoop
{
    tList evlist;
    tLock evlock;

    int epfd;

    struct epoll_event *evbuf;
    int max_ev_num;
    int is_init;

    pthread_t tid;
    int is_running;

} tEvLoop;

tEvStatus evloop_init(tEvLoop* loop, int max_ev_num);
tEvStatus evloop_uninit(tEvLoop* loop);
tEvStatus evloop_run(tEvLoop* loop);
tEvStatus evloop_break(tEvLoop* loop);

////////////////////////////////////////////////////////////////////////////////

typedef enum 
{
    EV_IO, 
    EV_TIMER,
    EV_SIGNAL,
    EV_ONCE,

    EV_TYPES

} tEvType;

#define EV_BASE_FIELDS \
    tEvType type;\
    int fd;\
    void* arg;\
    tEvLoop* loop;\
    int is_init;\
    int is_started

typedef struct tEvBase
{
    EV_BASE_FIELDS;

} tEvBase;

////////////////////////////////////////////////////////////////////////////////

typedef struct tEvOnce
{
    EV_BASE_FIELDS;

    void (*callback)(tEvLoop* loop, void* arg);

} tEvOnce;

typedef void (*tEvOnceCb)(tEvLoop* loop, void* arg);

tEvStatus ev_once(tEvLoop* loop, tEvOnceCb callback, void* arg);

////////////////////////////////////////////////////////////////////////////////

typedef struct tEvIo
{
    EV_BASE_FIELDS;

    void (*callback)(tEvLoop* loop, struct tEvIo* io, void* arg);

} tEvIo;

typedef void (*tEvIoCb)(tEvLoop* loop, struct tEvIo* io, void* arg);

tEvStatus evio_init(tEvIo* io, tEvIoCb callback, int fd, void* arg);
tEvStatus evio_start(tEvLoop* loop, tEvIo* io);
tEvStatus evio_stop(tEvLoop* loop, tEvIo* io);

////////////////////////////////////////////////////////////////////////////////

typedef enum 
{
    EV_TIMER_ONCE,
    EV_TIMER_PERIODIC,

    EV_TIMER_TYPES

} tEvTimerType;

typedef struct tEvTimer
{
    EV_BASE_FIELDS;

    tEvTimerType tm_type;
    int time_ms;

    void (*callback)(tEvLoop* loop, struct tEvTimer* tm, void* arg);

} tEvTimer;

typedef void (*tEvTimerCb)(tEvLoop* loop, struct tEvTimer* tm, void* arg);

tEvStatus evtm_init(tEvTimer* tm, tEvTimerCb callback, int time_ms, void* arg, tEvTimerType type);
tEvStatus evtm_start(tEvLoop* loop, tEvTimer* tm);
tEvStatus evtm_stop(tEvLoop* loop, tEvTimer* tm);
// tEvStatus evtm_pause(tEvLoop* loop, tEvTimer* tm);
// tEvStatus evtm_resume(tEvLoop* loop, tEvTimer* tm);
// tEvStatus evtm_restart(tEvLoop* loop, tEvTimer* tm);

////////////////////////////////////////////////////////////////////////////////

typedef struct tEvSignal
{
    EV_BASE_FIELDS;

    int signum;

    void (*callback)(tEvLoop* loop, struct tEvSignal* sig, void* arg);

} tEvSignal;

typedef void (*tEvSignalCb)(tEvLoop* loop, struct tEvSignal* sig, void* arg);

tEvStatus evsig_init(tEvSignal* sig, tEvSignalCb callback, int signum, void* arg);
tEvStatus evsig_start(tEvLoop* loop, tEvSignal* sig);
tEvStatus evsig_stop(tEvLoop* loop, tEvSignal* sig);

////////////////////////////////////////////////////////////////////////////////


#endif //_EVENTS_H_