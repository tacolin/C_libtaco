#ifndef _NEW_TIMER_H_
#define _NEW_TIMER_H_

#include "basic.h"
#include <signal.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////

#define TIMER_NAME_SIZE 20

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    TIMER_OK = 0,
    TIMER_ERROR = -1

} tTimerStatus;

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

tTimerStatus timer_init(tTimer* timer, char* name, int timeout_ms,
                        tTimerExpiredFn expired_fn, void* arg, tTimerType type);

tTimerStatus timer_uninit(tTimer* timer);
tTimerStatus timer_start(tTimer* timer);
tTimerStatus timer_stop(tTimer* timer);

static inline tTimerState timer_state(tTimer* timer)
{
    check_if(timer == NULL, return TIMER_STOPPED, "timer is null");
    return timer->state;
}

tTimerStatus timer_system_init(void);
tTimerStatus timer_system_uninit(void);

#endif //_NEW_TIMER_H_
