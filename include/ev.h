#ifndef _EV_H_
#define _EV_H_

#include <sys/epoll.h>      // For epoll
#include <sys/timerfd.h>    // For timerfd

#include "basic.h"
#include "list.h"
#include "lock.h"

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    EV_OK = 0,
    EV_ERROR = -1

} tEvStatus;

typedef enum
{
    EV_TIMER_ONCE,
    EV_TIMER_PERIODIC,

    EV_TIMER_TYPES

} tEvTimerType;

typedef tEvStatus (*tEvCallback)(int fd, void* arg);

typedef struct tEvIo
{
    int         fd;
    void*       arg
    tEvCallback callback;

} tEvIo;

typedef struct tEvTimer
{
    int fd;
    void* arg;
    int time_ms;
    tEvTimerType type;
    tEvCallback callback;

} tEvTimer;

typedef struct tEvSignal
{
    int         fd;
    int         signum;
    void*       arg
    tEvCallback callback;

} tEvSignal;

typedef struct tEvSystem
{

} tEvSystem;

////////////////////////////////////////////////////////////////////////////////

tEvStatus ev_system_init(tEvSystem* sys);
tEvStatus ev_system_uninit(tEvSystem* sys);

tEvStatus ev_system_run(tEvSystem* sys);
tEvStatus ev_system_stop(tEvSystem* sys);

tEvStatus ev_io_init(tEvIo *ep, int fd, void* arg, tEvCallback cb);
tEvStatus ev_io_start(tEvSystem* sys, tEvIo *ep);
tEvStatus ev_io_stop(tEvSystem* sys, tEvIo *ep);

tEvStatus ev_timer_init(tEvTimer *ep, int time_ms, void* arg, tEvTimerType type,  tEvCallback cb);
tEvStatus ev_timer_start(tEvSystem* sys, tEvTimer *ep);
tEvStatus ev_timer_stop(tEvSystem* sys, tEvTimer *ep);

tEvStatus ev_signal_init(tEvSignal *ep, int sig_num, void* arg, tEvCallback cb);
tEvStatus ev_signal_start(tEvSystem* sys, tEvSignal *ep);
tEvStatus ev_signal_stop(tEvSystem* sys, tEvSignal *ep);

tEvStatus ev_send(void* arg, tEvCallback cb); // thread-unsafe

////////////////////////////////////////////////////////////////////////////////

tEvStatus ep_safe_send(void* arg, tEvCallback cb); // thread-safe

#endif //_EV_H_
