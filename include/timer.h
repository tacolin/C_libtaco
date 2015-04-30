#ifndef _TIMER_H_
#define _TIMER_H_

#include "basic.h"
#include <signal.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////

#define TIMER_NAME_SIZE 20

#define TIMER_OK (0)
#define TIMER_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    TIMER_RUNNING,
    TIMER_STOPPED,

    TIMER_STATES

} tTimerState;

typedef enum
{
    TIMER_ONCE,
    TIMER_PERIODIC,

    TIMER_TYPES

} tTimerType;

typedef void (*tTimerExpiredFn)(void* arg);

typedef struct tTimer
{
    char name[TIMER_NAME_SIZE+1];
    int  timeout_ms; // ms
    void* arg;

    tTimerExpiredFn expired_fn;

    int guard_code;

    timer_t timer_id;
    struct sigevent ev;

    tTimerType type;
    tTimerState state;

} tTimer;

////////////////////////////////////////////////////////////////////////////////

int timer_init(tTimer* timer, char* name, int timeout_ms,
                        tTimerExpiredFn expired_fn, void* arg, tTimerType type);

int timer_uninit(tTimer* timer);
int timer_start(tTimer* timer);
int timer_stop(tTimer* timer);

static inline tTimerState timer_state(tTimer* timer)
{
    check_if(timer == NULL, return TIMER_STOPPED, "timer is null");
    return timer->state;
}

int timer_system_init(void);
int timer_system_uninit(void);

#endif //_TIMER_H_
