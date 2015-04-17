#ifndef _OLD_EVENTS_H_
#define _OLD_EVENTS_H_

#include <sys/select.h>
#include "basic.h"

////////////////////////////////////////////////////////////////////////////////

#define OEV_TICK_MS (100)

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    OEV_OK = 0,
    OEV_ERROR = -1

} tOevStatus;

typedef enum
{
    OEV_TIMER_ONESHOT,
    OEV_TIMER_PERIODIC,

    OEV_TIMER_TYPES

} tOevTimerType;

typedef enum
{
    OEV_IO,
    OEV_TIMER,
    OEV_ONCE,

    OEV_TYPES

} tOevType;

typedef enum
{
    OEV_IO_START,
    OEV_IO_STOP,

    OEV_TM_START,
    OEV_TM_STOP,
    OEV_TM_RESTART,
    OEV_TM_PAUSE,
    OEV_TM_RESUME,

    OEV_ONCE_ONCE,

    OEV_ACTIONS

} tOevAction;

////////////////////////////////////////////////////////////////////////////////

struct tOevLoop;

typedef int (*tOevCallback)(struct tOevLoop* loop, void* ev, void* arg);

typedef struct tOevQueueObj
{
    tOevType type;
    tOevAction action;
    void* ev;

    struct tOevQueueObj* next;

} tOevQueueObj;

typedef struct tOevQueue
{
    tOevQueueObj* head;
    tOevQueueObj* tail;

    int max_obj_num;
    int curr_obj_num;

    pthread_mutex_t lock;

    int is_init;

} tOevQueue;

typedef struct tOevIo
{
    int fd;

    tOevCallback callback;
    void* arg;

    pthread_t tid;

    int is_init;

    struct tOevIo* prev;
    struct tOevIo* next;

} tOevIo;

typedef struct tOevTimer
{
    tOevTimerType type;

    long rest_ms;
    long period_ms;

    tOevCallback callback;
    void* arg;

    pthread_t tid;

    int is_init;

    struct tOevTimer* prev;
    struct tOevTimer* next;

} tOevTimer;

typedef struct tOevOnce
{
    tOevCallback callback;
    void* arg;

    pthread_t tid;

    int is_init;

} tOevOnce;

typedef struct tOevLoop
{
    tOevTimer* timerlist;
    struct tOevIo*    iolist;
    tOevQueue  once_queue;

    tOevQueue  inter_thread_queue; // for inter_thread communications

    int is_init;
    int is_running;
    int is_still_running; // for inter_thread oevloop_break()

    pthread_t tid;

} tOevLoop;

////////////////////////////////////////////////////////////////////////////////

tOevStatus oevloop_init(tOevLoop* loop);
tOevStatus oevloop_uninit(tOevLoop* loop);

tOevStatus oevloop_run(tOevLoop* loop);
tOevStatus oevloop_break(tOevLoop* loop);

////////////////////////////////////////////////////////////////////////////////

tOevStatus oevio_init(tOevIo* io, int fd, tOevCallback callback, void* arg);
tOevStatus oevio_start(tOevLoop* loop, tOevIo* io);
tOevStatus oevio_stop(tOevLoop* loop, tOevIo* io);

////////////////////////////////////////////////////////////////////////////////

tOevStatus oevtm_init(tOevTimer* tm, tOevCallback callback, int period_ms, void* arg, tOevTimerType type);
tOevStatus oevtm_start(tOevLoop* loop, tOevTimer* tm);
tOevStatus oevtm_stop(tOevLoop* loop, tOevTimer* tm);

tOevStatus oevtm_restart(tOevLoop* loop, tOevTimer* tm);
tOevStatus oevtm_pause(tOevLoop* loop, tOevTimer* tm);
tOevStatus oevtm_resume(tOevLoop* loop, tOevTimer* tm);

////////////////////////////////////////////////////////////////////////////////

tOevStatus oev_once(tOevLoop* loop, tOevCallback callback, void* arg);

#endif //_OLD_EVENTS_H_
