#include "timer.h"
#include "lock.h"
#include "list.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#define TIMER_GUARD_CODE (0x5566)

////////////////////////////////////////////////////////////////////////////////

static int _system_init_flag = 0;

static tList _running_list;
static tLock _running_lock;

static struct sigaction _action = {};

////////////////////////////////////////////////////////////////////////////////

static void _cleanRunningTimer(void* arg)
{
    tTimer *timer = (tTimer*)arg;
    if (timer == NULL) return;

    timer_delete(timer->timer_id);

    timer->timeout_ms = 0;
    timer->expired_fn = NULL;
    timer->arg        = 0;
    timer->state      = TIMER_STOPPED;

    return;
}

// static void _defaultExpiredFn(union sigval val)
static void _defaultExpiredFn(int sig, siginfo_t *si, void *uc)
{
    tTimer* timer = (tTimer*)(si->si_value.sival_ptr);
    check_if(timer == NULL, return, "timer is null");

    if (timer->type == TIMER_ONCE)
    {
        timer->state = TIMER_STOPPED;

        lock_enter(&_running_lock);
        list_remove(&_running_list, timer);
        lock_exit(&_running_lock);

        timer_delete(timer->timer_id);
        dprint("delete");
    }

    timer->expired_fn(timer->arg);
    return;
}

int timer_init(tTimer *timer, char* name, int timeout_ms,
                        tTimerExpiredFn expired_fn, void* arg,
                        tTimerType type)
{
    check_if(_system_init_flag == 0, return TIMER_FAIL, "timer system is not initilized");
    check_if(timer == NULL, return TIMER_FAIL, "timer is null");
    check_if(expired_fn == NULL, return TIMER_FAIL, "expired_fn is null");

    memset(timer, 0, sizeof(tTimer));
    if (name)
    {
        snprintf(timer->name, TIMER_NAME_SIZE, "%s", name);
    }

    timer->timeout_ms = timeout_ms;
    timer->expired_fn = expired_fn;
    timer->arg        = arg;
    timer->state      = TIMER_STOPPED;
    timer->guard_code = TIMER_GUARD_CODE;
    timer->type       = (type == TIMER_PERIODIC) ? TIMER_PERIODIC : TIMER_ONCE;

    // timer->ev.sigev_value.sival_ptr = timer;
    // timer->ev.sigev_notify          = SIGEV_THREAD;
    // timer->ev.sigev_notify_function = _defaultExpiredFn;

    timer->ev.sigev_value.sival_ptr = timer;
    timer->ev.sigev_notify          = SIGEV_SIGNAL;
    timer->ev.sigev_signo           = SIGUSR1;

    return TIMER_OK;
}

int timer_uninit(tTimer* timer)
{
    check_if(_system_init_flag == 0, return TIMER_FAIL, "timer system is not initilized");
    check_if(timer == NULL, return TIMER_FAIL, "timer is null");

    check_if((timer->guard_code == TIMER_GUARD_CODE)
             && (timer->state == TIMER_RUNNING),
             return TIMER_FAIL, "timer(%s) is still running", timer->name);

    memset(timer, 0, sizeof(tTimer));

    return TIMER_OK;
}

int timer_start(tTimer *timer)
{
    check_if(_system_init_flag == 0, return TIMER_FAIL, "timer system is not initilized");

    check_if(timer == NULL, return TIMER_FAIL, "timer is null");

    check_if(timer->guard_code != TIMER_GUARD_CODE, return TIMER_FAIL,
             "this timer(%s) is not initialized, can not be started",
             timer->name);

    check_if(timer->state == TIMER_RUNNING, return TIMER_FAIL,
             "timer(%s) is running, can not be started again",
             timer->name);

    struct itimerspec timeval = {};
    timeval.it_value.tv_sec  = timer->timeout_ms / 1000;
    timeval.it_value.tv_nsec = (timer->timeout_ms % 1000) * 1000 * 1000;

    if (timer->type == TIMER_PERIODIC)
    {
        timeval.it_interval = timeval.it_value;
    }

    int ret = timer_create(CLOCK_REALTIME, &timer->ev, &timer->timer_id);
    check_if(ret < 0, return TIMER_FAIL, "timer_create failed");

    ret = timer_settime(timer->timer_id, 0, &timeval, NULL);
    check_if(ret < 0, return TIMER_FAIL, "timer_settime failed");

    lock_enter(&_running_lock);
    int list_ret = list_append(&_running_list, timer);
    lock_exit(&_running_lock);
    if (list_ret != LIST_OK)
    {
        memset(&timeval, 0, sizeof(struct itimerspec));
        timer_settime(timer->timer_id, 0, &timeval, NULL);
        return TIMER_FAIL;
    }

    timer->state = TIMER_RUNNING;

    return TIMER_OK;
}

int timer_stop(tTimer *timer)
{
    check_if(_system_init_flag == 0, return TIMER_FAIL, "timer system is not initilized");
    check_if(timer == NULL, return TIMER_FAIL, "timer is null");
    check_if(timer->guard_code != TIMER_GUARD_CODE, return TIMER_FAIL,
             "this timer(%s) is not initialized, can not be started",
             timer->name);
    check_if(timer->state == TIMER_STOPPED, return TIMER_FAIL,
             "timer(%s) is stopped, can not be started again", timer->name);

    struct itimerspec timeval = {};
    timer_settime(timer->timer_id, 0, &timeval, NULL);

    lock_enter(&_running_lock);
    int ret = list_remove(&_running_list, timer);
    lock_exit(&_running_lock);
    if (ret != LIST_OK)
    {
        derror("remove timer(%s) from running list failed", timer->name);
    }

    dprint("delete timer");
    timer_delete(timer->timer_id);

    timer->state = TIMER_STOPPED;
    return TIMER_OK;
}

int timer_system_init(void)
{
    if (_system_init_flag == 0)
    {
        list_init(&_running_list, _cleanRunningTimer);
        lock_init(&_running_lock);

        _action.sa_flags     = SA_SIGINFO;
        _action.sa_sigaction = _defaultExpiredFn;

        sigemptyset(&_action.sa_mask);
        sigaction(SIGUSR1, &_action, NULL);

        _system_init_flag = 1;

        return TIMER_OK;
    }
    else
    {
        derror("timer system has already been initialized!");
    }

    return TIMER_FAIL;
}

int timer_system_uninit(void)
{
    if ( _system_init_flag == 1 )
    {
        list_clean(&_running_list);
        lock_uninit(&_running_lock);

        _system_init_flag = 0;

        return TIMER_OK;
    }
    else
    {
        derror("timer system is not initialized!");
    }

    return TIMER_FAIL;
}
