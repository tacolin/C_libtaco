#include "events.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static void _cleanEv(void* content)
{
    tEv* ev = (tEv*)content;
    check_if(ev == NULL, return, "content is null");
    check_if(ev->loop == NULL, return, "ev->loop is null");

    if (ev->type == EV_TIMER)
    {
        tEvTimer* tm = (tEvTimer*)ev;

        memset(&tm->rest_time, 0, sizeof(struct itimerspec));
        timerfd_settime(tm->fd, 0, &tm->rest_time, NULL);

        struct epoll_event tmp = {};
        epoll_ctl(tm->loop->epfd, EPOLL_CTL_DEL, tm->fd, &tmp);

        close(tm->fd);
        tm->fd = -1;
    }
    else if (ev->type == EV_ONCE)
    {
        tEvOnce* once = (tEvOnce*)ev;

        struct epoll_event tmp = {};
        epoll_ctl(once->loop->epfd, EPOLL_CTL_DEL, once->fd, &tmp);

        close(once->fd);
        free(once);
    }
    else if (ev->type == EV_SIGNAL)
    {
        tEvSignal* sig = (tEvTimer*)ev;

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, sig->signum);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        struct epoll_event tmpev = {};
        epoll_ctl(sig->loop->epfd, EPOLL_CTL_DEL, sig->fd, &tmpev);

        close(sig->fd);
        sig->fd = -1;
    }

    return;
}

static void _handleInterThreadEv(tEvLoop* loop, tEv* input_ev, void* arg)
{
    check_if(loop == NULL, return, "loop is null");
    check_if(input_ev == NULL, return, "ev is null");
    check_if(loop->is_init != 1, return, "loop is not init yet");

    eventfd_t val;
    eventfd_read(input_ev->fd, &val);

    tEv* ev;
    while ((ev = queue_pop(&loop->inter_thread_queue)) && loop->is_running)
    {
        if (ev->type == EV_IO)
        {
            if (ev->action == EV_ACT_IO_START)
            {
                evio_start(loop, ev);
            }
            else if (ev->action == EV_ACT_IO_STOP)
            {
                evio_stop(loop, ev);
            }
            else
            {
                derror("unknown ev io action = %d", ev->action);
                break;
            }
        }
        else if (ev->type == EV_SIGNAL)
        {
            if (ev->action == EV_ACT_SIGNAL_START)
            {
                evsig_start(loop, ev);
            }
            else if (ev->action == EV_ACT_SIGNAL_STOP)
            {
                evsig_stop(loop, ev);
            }
            else
            {
                derror("unknown ev signal action = %d", ev->action);
                break;
            }
        }
        else if (ev->type == EV_ONCE)
        {
            if (ev->action == EV_ACT_ONCE)
            {
                ev_once(loop, ev->callback, ev->arg);
                free(ev);
                continue;
            }
            else
            {
                derror("unknown ev once action = %d", ev->action);
                break;
            }
        }
        else if (ev->type == EV_TIMER)
        {
            if (ev->action == EV_ACT_TIMER_START)
            {
                evtm_start(loop, ev);
            }
            else if (ev->action == EV_ACT_TIMER_STOP)
            {
                evtm_stop(loop, ev);
            }
            else if (ev->action == EV_ACT_TIMER_PAUSE)
            {
                evtm_pause(loop, ev);
            }
            else if (ev->action == EV_ACT_TIMER_RESUME)
            {
                evtm_resume(loop, ev);
            }
            else if (ev->action == EV_ACT_TIMER_RESTART)
            {
                evtm_restart(loop, ev);
            }
            else
            {
                derror("unknown ev timer action = %d", ev->action);
                break;
            }
        }

        ev->action = EV_ACT_INVALID;
    }

    return;
}

static void _handleEv(tEvLoop* loop, tEv* ev)
{
    check_if(loop == NULL, return, "loop is null");
    check_if(loop->is_init != 1, return, "loop is not init yet");
    check_if(loop->is_running != 1, return, "loop is not running");
    check_if(ev == NULL, return, "ev is null");
    check_if(ev->is_init != 1, return, "ev is not init yet");
    check_if(ev->is_started != 1, return, "ev is not started yet");

    if (ev->type == EV_IO)
    {
        tEvIo* io = (tEvIo*)ev;

        io->callback(loop, io, io->arg);
    }
    else if (ev->type == EV_SIGNAL)
    {
        tEvSignal* sig               = (tEvSignal*)ev;
        struct signalfd_siginfo info = {};
        ssize_t info_size            = sizeof(info);

        ssize_t readlen = read(sig->fd, &info, sizeof(info));
        check_if(readlen != info_size, return, "read failed");
        check_if(info.ssi_signo != sig->signum, return, "sig->signum (%d) is not equal to ssi_signo (%d)", sig->signum, info.ssi_signo);

        sig->callback(loop, sig, sig->arg);
    }
    else if (ev->type == EV_ONCE)
    {
        tEvOnce* once = (tEvOnce*)ev;
        eventfd_t val;

        int check = eventfd_read(once->fd, &val);
        check_if(check < 0, return, "eventfd_read failed");

        once->callback(loop, once, once->arg);

        struct epoll_event tmpev = {};
        epoll_ctl(loop->epfd, EPOLL_CTL_DEL, once->fd, &tmpev);

        loop->curr_ev_num--;

        close(once->fd);
        free(once);
    }
    else if (ev->type == EV_TIMER)
    {
        tEvTimer* tm = (tEvTimer*)ev;
        uint64_t val;
        ssize_t val_size = sizeof(val);

        ssize_t readlen = read(tm->fd, &val, val_size);
        check_if(readlen != val_size, return, "read failed");

        tm->callback(loop, tm, tm->arg);

        if (tm->timer_type == EV_TIMER_ONESHOT)
        {
            evtm_stop(loop, tm);
        }
    }
    else
    {
        derror("unknown ev type = %d", ev->type);
    }

    return;
}

static tEvStatus _saveInterThreadAction(tEvLoop* loop, tEv* ev, tEvAction action)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return, "loop is not init yet");
    check_if(ev == NULL, return EV_ERROR, "ev is null");
    check_if(ev->is_init != 1, return, "ev is not init yet");

    ev->action = action;

    int ret = queue_push(&loop->inter_thread_queue, ev);
    check_if(ret != QUEUE_OK, return EV_ERROR, "queue_put failed");

    eventfd_t val = loop->queue_fd;
    eventfd_write(loop->queue_fd, val);

    return EV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evloop_init(tEvLoop* loop, int max_ev_num)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(max_ev_num <= 0, return EV_ERROR, "max_ev_num = %d invalid", max_ev_num);

    memset(loop, 0, sizeof(tEvLoop));

    int queue_ret = queue_init(&loop->inter_thread_queue, max_ev_num, _cleanEv,
                               QUEUE_UNSUSPEND, QUEUE_UNSUSPEND);
    check_if(queue_ret != QUEUE_OK, return EV_ERROR, "queue_init failed");

    loop->epfd = epoll_create(max_ev_num);
    check_if(loop->epfd <= 0, return EV_ERROR, "epoll_create failed");

    loop->max_ev_num = max_ev_num;
    loop->curr_ev_num = 0;
    loop->is_running = 0;
    loop->is_still_running = 0;
    loop->tid = 0;
    loop->is_init = 1;

    return EV_OK;
}

tEvStatus evloop_uninit(tEvLoop* loop)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(loop->epfd < 0, return EV_ERROR, "loop->epfd = %d invalid", loop->epfd);

    if (loop->is_running)
    {
        evloop_break(loop);
    }

    queue_clean(&loop->inter_thread_queue);

    loop->is_init = 0;

    close(loop->epfd);
    loop->epfd = -1;

    loop->max_ev_num = 0;
    loop->curr_ev_num = 0;
    loop->is_running = 0;
    // loop->tid = 0;

    return EV_OK;
}

tEvStatus evloop_run(tEvLoop* loop)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(loop->epfd < 0, return EV_ERROR, "loop->epfd = %d invalid", loop->epfd);
    check_if(loop->is_running != 0, return EV_ERROR, "loop is already running");

    loop->is_running = 1;
    loop->is_still_running = 1;
    loop->tid = pthread_self();

    struct epoll_event evbuf[loop->max_ev_num];
    memset(evbuf, 0, sizeof(struct epoll_event) * loop->max_ev_num);

    loop->queue_fd = eventfd(0, 0);
    check_if(loop->queue_fd < 0, return EV_ERROR, "eventfd failed");

    tEvIo io = {};
    evio_init(&io, _handleInterThreadEv, loop->queue_fd, NULL);
    evio_start(loop, &io);

    eventfd_t val = loop->queue_fd;
    eventfd_write(loop->queue_fd, val);

    int i;
    int ev_num;
    tEv* ev;
    while (loop->is_running)
    {
        ev_num = epoll_wait(loop->epfd, evbuf, loop->max_ev_num, EV_TICK_MS);
        check_if(ev_num < 0, goto _ERROR, "epoll_wait failed");

        if (ev_num > 0)
        {
            for (i=0; i<ev_num && loop->is_running; i++)
            {
                ev = (tEv*)(evbuf[i].data.ptr);
                check_if(ev == NULL, goto _ERROR, "epoll ev is null");
                _handleEv(loop, ev);
            }
        }
    }

    evio_stop(loop, &io);
    close(loop->queue_fd);
    loop->queue_fd = -1;
    loop->is_still_running = 0;
    loop->tid = 0;
    return EV_OK;

_ERROR:
    evio_stop(loop, &io);
    close(loop->queue_fd);
    loop->queue_fd = -1;
    loop->is_still_running = 0;
    loop->tid = 0;
    return EV_ERROR;
}

tEvStatus evloop_break(tEvLoop* loop)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(loop->is_running != 1, return EV_ERROR, "loop is not running");

    loop->is_running = 0;

    if (pthread_self() != loop->tid)
    {
        // break triggered by another thread
        // wait for evloop totally break
        struct timeval timeout;
        while (loop->is_still_running)
        {
            timeout.tv_sec  = EV_TICK_MS / 1000;
            timeout.tv_usec = (EV_TICK_MS % 1000) * 1000;
            select(0, NULL, NULL, NULL, &timeout);
        }

        // for safety
        timeout.tv_sec  = EV_TICK_MS / 1000;
        timeout.tv_usec = (EV_TICK_MS % 1000) * 1000;
        select(0, NULL, NULL, NULL, &timeout);
    }

    return EV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evio_init(tEvIo* io, tEvCallback callback, int fd, void* arg)
{
    check_if(io == NULL, return EV_ERROR, "io is null");
    check_if(callback == NULL, return EV_ERROR, "callback is null");
    check_if(fd < 0, return EV_ERROR, "fd = %d invalid", fd);

    memset(io, 0, sizeof(tEvIo));

    io->type = EV_IO;
    io->fd = fd;
    io->arg = arg;
    io->callback = callback;
    io->loop = NULL;
    io->is_started = 0;
    io->action = EV_ACT_INVALID;

    io->is_init = 1;
    return EV_OK;
}

tEvStatus evio_start(tEvLoop* loop, tEvIo* io)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(io == NULL, return EV_ERROR, "io is null");
    check_if(io->is_init != 1, return EV_ERROR, "io is not init yet");
    check_if(io->is_started != 0, return EV_ERROR, "io is already started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, io, EV_ACT_IO_START);
    }

    check_if(loop->curr_ev_num >= loop->max_ev_num, return EV_ERROR, "curr_ev_num = %d >= max_ev_num = %d", loop->curr_ev_num, loop->max_ev_num);

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = io,
    };

    int check = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, io->fd, &tmp);
    check_if(check < 0, return EV_ERROR, "epoll_ctl add failed");

    loop->curr_ev_num++;
    io->loop = loop;
    io->is_started = 1;

    return EV_OK;
}

tEvStatus evio_stop(tEvLoop* loop, tEvIo* io)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(io == NULL, return EV_ERROR, "io is null");
    check_if(io->is_init != 1, return EV_ERROR, "io is not init yet");
    check_if(io->is_started != 1, return EV_ERROR, "io is not started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, io, EV_ACT_IO_STOP);
    }

    io->is_started = 0;

    struct epoll_event tmp = {};
    int check = epoll_ctl(loop->epfd, EPOLL_CTL_DEL, io->fd, &tmp);

    loop->curr_ev_num--;

    check_if(check < 0, return EV_ERROR, "epoll_ctl EPOLL_CTL_DEL failed");

    return EV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evsig_init(tEvSignal* sig, tEvCallback callback, int signum, void* arg)
{
    check_if(sig == NULL, return EV_ERROR, "sig is null");
    check_if(callback == NULL, return EV_ERROR, "callback is null");
    check_if(signum <= 0, return EV_ERROR, "signum = %d invalid", signum);

    memset(sig, 0, sizeof(tEvSignal));

    sig->loop = NULL;
    sig->type = EV_SIGNAL;
    sig->fd = -1;
    sig->arg = arg;
    sig->callback = callback;
    sig->is_started = 0;
    sig->action = EV_ACT_INVALID;
    sig->signum = signum;

    sig->is_init = 1;

    return EV_OK;
}

tEvStatus evsig_start(tEvLoop* loop, tEvSignal* sig)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(sig == NULL, return EV_ERROR, "sig is null");
    check_if(sig->is_init != 1, return EV_ERROR, "sig is not init yet");
    check_if(sig->is_started != 0, return EV_ERROR, "sig is already started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, sig, EV_ACT_SIGNAL_START);
    }

    check_if(loop->curr_ev_num >= loop->max_ev_num, return EV_ERROR, "curr_ev_num = %d >= max_ev_num = %d", loop->curr_ev_num, loop->max_ev_num);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig->signum);

    // Block the signals thet we handle using signalfd(), so they don't
    // cause signal handlers or default signal actions to execute.
    int check = sigprocmask(SIG_BLOCK, &mask, NULL);
    check_if(check < 0, return EV_ERROR, "sigprocmask failed");

    sig->fd = signalfd(-1, &mask, 0);
    check_if(sig->fd <= 0, return EV_ERROR, "signalfd failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = sig,
    };
    check = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, sig->fd, &tmp);
    check_if(check < 0, goto _ERROR, "epoll_ctl add failed");

    loop->curr_ev_num++;

    sig->loop = loop;
    sig->is_started = 1;

    return EV_OK;

_ERROR:
    if (sig->fd > 0)
    {
        close(sig->fd);
        sig->fd = -1;
    }
    return EV_ERROR;
}

tEvStatus evsig_stop(tEvLoop* loop, tEvSignal* sig)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(sig == NULL, return EV_ERROR, "sig is null");
    check_if(sig->is_init != 1, return EV_ERROR, "sig is not init yet");
    check_if(sig->is_started != 1, return EV_ERROR, "sig is not started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, sig, EV_ACT_SIGNAL_STOP);
    }

    // unblock the signal handling, give the control back to linux
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig->signum);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    struct epoll_event tmpev = {};
    int check = epoll_ctl(loop->epfd, EPOLL_CTL_DEL, sig->fd, &tmpev);

    loop->curr_ev_num--;

    close(sig->fd);
    sig->fd = -1;

    check_if(check < 0, return EV_ERROR, "epoll_ctl EPOLL_CTL_DEL failed");

    return EV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus ev_once(tEvLoop* loop, tEvCallback callback, void* arg)
{
    check_if(callback == NULL, return EV_ERROR, "callback is null");
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");

    /////////////////////////////////////////////
    //  init
    /////////////////////////////////////////////
    tEvOnce* once = calloc(sizeof(tEvOnce), 1);

    once->type = EV_ONCE;
    once->arg = arg;
    once->callback = callback;
    once->is_started = 0;
    once->fd = -1;
    once->is_init = 1;

    /////////////////////////////////////////////
    //  start
    /////////////////////////////////////////////
    if (pthread_self() != loop->tid)
    {
        tEvStatus ret = _saveInterThreadAction(loop, once, EV_ACT_ONCE);
        if (ret != EV_OK)
        {
            free(once);
            return EV_ERROR;
        }
        return EV_OK;
    }

    int ep_chk = -1;
    int wr_chk = -1;

    check_if(loop->curr_ev_num >= loop->max_ev_num, goto _ERROR, "curr_ev_num = %d >= max_ev_num = %d", loop->curr_ev_num, loop->max_ev_num);

    once->fd = eventfd(0, 0);
    check_if(once->fd < 0, goto _ERROR, "eventfd failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = once,
    };
    ep_chk = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, once->fd, &tmp);
    check_if(ep_chk < 0, goto _ERROR, "epoll_ctl add failed");

    eventfd_t val = once->fd;
    wr_chk = eventfd_write(once->fd, val);
    check_if(wr_chk < 0, goto _ERROR, "eventfd_write failed");

    once->loop = loop;
    once->is_started = 1;

    return EV_OK;

_ERROR:
    if (ep_chk == 0)
    {
        epoll_ctl(loop->epfd, EPOLL_CTL_DEL, once->fd, &tmp);
    }
    if (once)
    {
        if (once->fd > 0)
        {
            close(once->fd);
        }
        free(once);
    }
    return EV_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evtm_init(tEvTimer* tm, tEvCallback callback, int period_ms, void* arg, tEvTimerType timer_type)
{
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(callback == NULL, return EV_ERROR, "callback is null");
    check_if(period_ms <= 0, return EV_ERROR, "period_ms = %d invalid", period_ms);

    memset(tm, 0, sizeof(tEvTimer));

    tm->loop = NULL;
    tm->type = EV_TIMER;
    tm->fd = -1;
    tm->arg = arg;
    tm->callback = callback;
    tm->is_started = 0;

    tm->period_ms = period_ms;
    tm->timer_type = timer_type;

    tm->is_init = 1;

    return EV_OK;
}

tEvStatus evtm_start(tEvLoop* loop, tEvTimer* tm)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return EV_ERROR, "tm is not init yet");
    check_if(tm->is_started != 0, return EV_ERROR, "tm has been already started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, tm, EV_ACT_TIMER_START);
    }

    check_if(loop->curr_ev_num >= loop->max_ev_num, return EV_ERROR, "curr_ev_num = %d >= max_ev_num = %d", loop->curr_ev_num, loop->max_ev_num);

    tm->fd = timerfd_create(CLOCK_MONOTONIC, 0);
    check_if(tm->fd < 0, return EV_ERROR, "timerfd_create failed");

    time_t sec  = tm->period_ms / 1000;
    long   nsec = (tm->period_ms % 1000) * 1000 * 1000;

    tm->rest_time.it_value.tv_sec = sec;
    tm->rest_time.it_value.tv_nsec = nsec;

    if (tm->timer_type == EV_TIMER_PERIODIC)
    {
        tm->rest_time.it_interval.tv_sec = sec;
        tm->rest_time.it_interval.tv_nsec = nsec;
    }

    int set_chk = timerfd_settime(tm->fd, 0, &tm->rest_time, NULL);
    check_if(set_chk < 0, goto _ERROR, "timerfd_settime failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = tm,
    };

    int ep_chk = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, tm->fd, &tmp);
    check_if(ep_chk < 0, goto _ERROR, "epoll_ctl add failed");

    loop->curr_ev_num++;

    tm->loop = loop;
    tm->is_started = 1;

    return EV_OK;

_ERROR:
    if (tm->fd > 0)
    {
        close(tm->fd);
        tm->fd = -1;
    }
    return EV_ERROR;
}

tEvStatus evtm_stop(tEvLoop* loop, tEvTimer* tm)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(tm->fd <= 0, return EV_ERROR, "fd = %d invalid", tm->fd);
    check_if(tm->is_init != 1, return EV_ERROR, "tm is not init yet");
    check_if(tm->is_started != 1, return EV_ERROR, "tm is not started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, tm, EV_ACT_TIMER_STOP);
    }

    tm->is_started = 0;

    // stop the timerfd in linux
    memset(&tm->rest_time, 0, sizeof(struct itimerspec));
    int set_chk = timerfd_settime(tm->fd, 0, &tm->rest_time, NULL);

    struct epoll_event tmp = {};
    int ep_chk = epoll_ctl(loop->epfd, EPOLL_CTL_DEL, tm->fd, &tmp);

    loop->curr_ev_num--;

    close(tm->fd);
    tm->fd = -1;

    check_if(set_chk < 0, return EV_ERROR, "timerfd_settime failed");
    check_if(ep_chk < 0, return EV_ERROR, "epoll_ctl EPOLL_CTL_DEL failed");

    return EV_OK;
}

tEvStatus evtm_pause(tEvLoop* loop, tEvTimer* tm)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(tm->fd <= 0, return EV_ERROR, "fd = %d invalid", tm->fd);
    check_if(tm->is_init != 1, return EV_ERROR, "tm is not init yet");
    check_if(tm->is_started != 1, return EV_ERROR, "tm is not started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, tm, EV_ACT_TIMER_PAUSE);
    }

    struct itimerspec tmp = {};
    int get_chk = timerfd_gettime(tm->fd, &tmp);
    check_if(get_chk < 0, return EV_ERROR, "timerfd_gettime failed");

    dprint("sec = %ld, nsec = %ld", tmp.it_value.tv_sec, tmp.it_value.tv_nsec);

    tEvStatus ret = evtm_stop(loop, tm);
    check_if(ret < 0, return EV_ERROR, "evtm_stop failed");

    tm->rest_time = tmp;

    return EV_OK;
}

tEvStatus evtm_resume(tEvLoop* loop, tEvTimer* tm)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return EV_ERROR, "tm is not init yet");
    check_if(tm->is_started != 0, return EV_ERROR, "tm has been already started");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, tm, EV_ACT_TIMER_RESUME);
    }

    check_if(loop->curr_ev_num >= loop->max_ev_num, return EV_ERROR, "curr_ev_num = %d >= max_ev_num = %d", loop->curr_ev_num, loop->max_ev_num);

    tm->fd = timerfd_create(CLOCK_MONOTONIC, 0);
    check_if(tm->fd < 0, return EV_ERROR, "timerfd_create failed");

    int set_chk = timerfd_settime(tm->fd, 0, &tm->rest_time, NULL);
    check_if(set_chk < 0, goto _ERROR, "timerfd_settime failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = tm,
    };

    int ep_chk = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, tm->fd, &tmp);
    check_if(ep_chk < 0, goto _ERROR, "epoll_ctl add failed");

    loop->curr_ev_num++;

    tm->loop = loop;
    tm->is_started = 1;

    return EV_OK;

_ERROR:
    if (tm->fd > 0)
    {
        close(tm->fd);
        tm->fd = -1;
    }
    return EV_ERROR;
}

tEvStatus evtm_restart(tEvLoop* loop, tEvTimer* tm)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return EV_ERROR, "tm is not init yet");

    if (pthread_self() != loop->tid)
    {
        return _saveInterThreadAction(loop, tm, EV_ACT_TIMER_RESTART);
    }

    if (tm->is_started)
    {
        tEvStatus ret = evtm_stop(loop, tm);
        check_if(ret != EV_OK, return EV_ERROR, "evtm_stop failed");
    }

    tEvStatus ret = evtm_start(loop, tm);
    check_if(ret != EV_OK, return EV_ERROR, "evtm_start failed");

    return EV_OK;
}
