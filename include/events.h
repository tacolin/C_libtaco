#ifndef _EVENTS_H_
#define _EVENTS_H_

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>
#include <signal.h>

////////////////////////////////////////////////////////////////////////////////

#include "basic.h"
#include "queue.h"

////////////////////////////////////////////////////////////////////////////////

#define EV_TICK_MS (50)

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    EV_OK = 0,
    EV_ERROR = -1

} tEvStatus;

typedef enum
{
    EV_IO,
    EV_TIMER,
    EV_SIGNAL,
    EV_ONCE,

    EV_TYPES

} tEvType;

typedef enum
{
    EV_TIMER_ONESHOT,
    EV_TIMER_PERIODIC,

    EV_TIMER_TYPES

} tEvTimerType;

typedef enum
{
    EV_ACT_INVALID = 0,

    EV_ACT_IO_START,
    EV_ACT_IO_STOP,

    EV_ACT_TIMER_START,
    EV_ACT_TIMER_STOP,
    EV_ACT_TIMER_PAUSE,
    EV_ACT_TIMER_RESUME,
    EV_ACT_TIMER_RESTART,

    EV_ACT_SIGNAL_START,
    EV_ACT_SIGNAL_STOP,

    EV_ACT_ONCE,

    EV_ACTIONS

} tEvAction;

////////////////////////////////////////////////////////////////////////////////

typedef struct tEvLoop
{
    int queue_fd;
    tQueue inter_thread_queue;

    int epfd;
    int max_ev_num;
    int curr_ev_num;

    int is_init;
    int is_running;
    int is_still_running;

    pthread_t tid;

} tEvLoop;

struct tEv;

typedef void (*tEvCallback)(tEvLoop* loop, struct tEv* ev, void* arg);

typedef struct tEv
{
    tEvLoop*    loop;
    tEvType     type;
    int         fd;
    void*       arg;
    tEvCallback callback;
    int         is_init;
    int         is_started;

    ///////////////////////////////////
    // for timer
    ///////////////////////////////////
    struct itimerspec rest_time;
    long period_ms;
    tEvTimerType timer_type;

    ///////////////////////////////////
    // for signal
    ///////////////////////////////////
    int signum;

    ///////////////////////////////////
    // for inter thread queue
    ///////////////////////////////////
    tEvAction action;

} tEv;

typedef tEv tEvIo;
typedef tEv tEvTimer;
typedef tEv tEvSignal;
typedef tEv tEvOnce;

////////////////////////////////////////////////////////////////////////////////

tEvStatus evloop_init(tEvLoop* loop, int max_ev_num);
tEvStatus evloop_uninit(tEvLoop* loop);
tEvStatus evloop_run(tEvLoop* loop);
tEvStatus evloop_break(tEvLoop* loop);

////////////////////////////////////////////////////////////////////////////////

tEvStatus ev_once(tEvLoop* loop, tEvCallback callback, void* arg);

tEvStatus evio_init(tEvIo* io, tEvCallback callback, int fd, void* arg);
tEvStatus evio_start(tEvLoop* loop, tEvIo* io);
tEvStatus evio_stop(tEvLoop* loop, tEvIo* io);

tEvStatus evsig_init(tEvSignal* sig, tEvCallback callback, int signum, void* arg);
tEvStatus evsig_start(tEvLoop* loop, tEvSignal* sig);
tEvStatus evsig_stop(tEvLoop* loop, tEvSignal* sig);

tEvStatus evtm_init(tEvTimer* tm, tEvCallback callback, int period_ms, void* arg, tEvTimerType timer_type);
tEvStatus evtm_start(tEvLoop* loop, tEvTimer* tm);
tEvStatus evtm_stop(tEvLoop* loop, tEvTimer* tm);
tEvStatus evtm_pause(tEvLoop* loop, tEvTimer* tm);
tEvStatus evtm_resume(tEvLoop* loop, tEvTimer* tm);
tEvStatus evtm_restart(tEvLoop* loop, tEvTimer* tm);

#endif //_EVENTS_H_
