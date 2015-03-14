#include "events.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static void _enterLock(tEvLoop* loop)
{
    // if caller thread is different with the evloop running thread, enter / exit lock
    // else do nothing
    if (loop->tid != pthread_self()) lock_enter(&loop->evlock);
}

static void _exitLock(tEvLoop* loop)
{
    if (loop->tid != pthread_self()) lock_exit(&loop->evlock);
}

////////////////////////////////////////////////////////////////////////////////

static void _cleanIoEv(tEvIo* io)
{
    // io fd should be maintained by user self, not by evloop system
    // do epoll ctrl deletion only
    struct epoll_event tmp = {};
    epoll_ctl(io->loop->epfd, EPOLL_CTL_DEL, io->fd, &tmp);

    return;
}

static void _cleanOnceEv(tEvOnce* once)
{
    // default callback will do epoll ctrl deletion

    // close eventfd and free memory
    close(once->fd);
    free(once);
    return;
}

static void _cleanTimerEv(tEvTimer* tm)
{
    // stop the timerfd in linux
    struct itimerspec timeval = {};
    timerfd_settime(tm->fd, 0, &timeval, NULL);

    // epoll ctrl deletion
    struct epoll_event tmp = {};
    epoll_ctl(tm->loop->epfd, EPOLL_CTL_DEL, tm->fd, &tmp);

    // close timerfd
    close(tm->fd);
    tm->fd = -1;
    return;
}

static void _cleanSignalEv(tEvSignal* sig)
{
    // unblock the signal handling, give the control back to linux
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig->signum);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    //  epoll ctrl deletion
    struct epoll_event tmpev = {};
    epoll_ctl(sig->loop->epfd, EPOLL_CTL_DEL, sig->fd, &tmpev);

    // close signal fd
    close(sig->fd);
    sig->fd = -1;
    return;
}

static void _cleanEv(void* content)
{
    check_if(content == NULL, return, "ev is null");

    tEvBase* base = (tEvBase*)content;
    switch (base->type)
    {
        case EV_SIGNAL:
            _cleanSignalEv((tEvSignal*)base);
            break;

        case EV_TIMER:
            _cleanTimerEv((tEvTimer*)base);
            break;

        case EV_ONCE:
            _cleanOnceEv((tEvOnce*)base);
            break;

        case EV_IO:
            _cleanIoEv((tEvIo*)base);
            break;

        default:
            derror("unknown ev type = %d", base->type);
            break;
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////

static void _procIoEv(tEvLoop* loop, tEvIo* io)
{
    io->callback(loop, io, io->arg);
    return;
}

static void _procTimerEv(tEvLoop* loop, tEvTimer* tm)
{
    uint64_t val;
    ssize_t val_size = sizeof(val);

    ssize_t readlen = read(tm->fd, &val, val_size);
    check_if(readlen != val_size, return, "read failed");

    if (tm->tm_type == EV_TIMER_ONSHOT)
    {
        _cleanTimerEv(tm); // clean function will not affect callback and arg
        list_remove(&loop->evlist, tm);
        tm->is_started = 0;
    }

    tm->callback(loop, tm, tm->arg);
    return;
}

static void _procSignalEv(tEvLoop* loop, tEvSignal* sig)
{
    struct signalfd_siginfo info = {};
    ssize_t info_size            = sizeof(info);

    ssize_t readlen = read(sig->fd, &info, sizeof(info));
    check_if(readlen != info_size, return, "read failed");
    check_if(info.ssi_signo != sig->signum, return, "sig->signum (%d) is not equal to ssi_signo (%d)", sig->signum, info.ssi_signo);

    sig->callback(loop, sig, sig->arg);
    return;
}

static void _procOnceEv(tEvLoop* loop, tEvOnce* once)
{
    eventfd_t val;

    int check = eventfd_read(once->fd, &val);
    check_if(check < 0, return, "eventfd_read failed");

    list_remove(&loop->evlist, once); // no need to do  lock protection for once event

    once->callback(loop, once->arg);

    _cleanOnceEv(once); //_cleanOnceEv will free memory, you should do it after callback

    return;
}

static void _procEv(tEvLoop* loop, tEvBase* base)
{
    switch (base->type)
    {
        case EV_ONCE:
            _procOnceEv(loop, (tEvOnce*)base);
            break;

        case EV_IO:
            _procIoEv(loop, (tEvIo*)base);
            break;

        case EV_TIMER:
            _procTimerEv(loop, (tEvTimer*)base);
            break;

        case EV_SIGNAL:
            _procSignalEv(loop, (tEvSignal*)base);
            break;

        default:
            derror("unknown ev type = %d", base->type);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evloop_init(tEvLoop* loop, int max_ev_num)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(max_ev_num <= 0, return EV_ERROR, "max_ev_num = %d invalid", max_ev_num);

    tListStatus list_ret = list_init(&loop->evlist, _cleanEv);
    check_if(list_ret != LIST_OK, return EV_ERROR, "list init failed");

    tLockStatus lock_ret = lock_init(&loop->evlock);
    check_if(lock_ret != LOCK_OK, return EV_ERROR, "lock init failed");

    loop->max_ev_num = max_ev_num;

    loop->epfd = epoll_create(max_ev_num);
    check_if(loop->epfd <= 0, return EV_ERROR, "epoll_create failed");

    loop->evbuf = calloc(sizeof(struct epoll_event), max_ev_num);
    check_if(loop->evbuf == NULL, return EV_ERROR, "calloc failed");

    loop->is_running = 0;
    loop->tid        = 0;
    loop->is_init    = 1;

    return EV_OK;
}

tEvStatus evloop_uninit(tEvLoop* loop)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");

    if (loop->is_running) evloop_break(loop); // evloop_break will clean evlist and reset tid

    _enterLock(loop);
    list_clean(&loop->evlist);
    _exitLock(loop);

    lock_uninit(&loop->evlock);

    if (loop->evbuf) free(loop->evbuf);

    if (loop->epfd > 0)
    {
        close(loop->epfd);
        loop->epfd = -1;
    }

    loop->max_ev_num = 0;
    loop->tid        = 0;
    loop->is_init    = 0;

    return EV_OK;
}

tEvStatus evloop_run(tEvLoop* loop)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(loop->is_running != 0, return EV_ERROR, "loop has been already running");

    loop->is_running = 1;
    loop->tid        = pthread_self();

    int ev_num;
    int i;
    tEvBase* ev;
    while (loop->is_running)
    {
        ev_num = epoll_wait(loop->epfd, loop->evbuf, loop->max_ev_num, 1);
        check_if(ev_num < 0, goto _ERROR, "epoll_wait failed");

        if (ev_num == 0) continue;

        // ev_num > 0
        for (i=0; i<ev_num && loop->is_running; i++)
        {
            if (loop->evbuf[i].events & EPOLLIN)
            {
                ev = (tEvBase*)(loop->evbuf[i].data.ptr);
                check_if(ev == NULL, goto _ERROR, "epoll ev ptr is null");

                lock_enter(&loop->evlock); // we must enterlock here, so do not use _enterLock();
                _procEv(loop, ev);
                lock_exit(&loop->evlock);
            }
        }
    }

    return EV_OK;

_ERROR:
    return EV_ERROR;
}

tEvStatus evloop_break(tEvLoop* loop)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(loop->is_running != 1, return EV_ERROR, "loop is not running");

    loop->is_running = 0;

    _enterLock(loop); // wait for loop exit
    _exitLock(loop);

    return EV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evsig_init(tEvSignal* sig, tEvSignalCb callback, int signum, void* arg)
{
    check_if(sig == NULL, return EV_ERROR, "sig is null");
    check_if(callback == NULL, return EV_ERROR, "callback is null");
    check_if(signum <= 0 , return EV_ERROR, "signum = %d invalid", signum);

    sig->type       = EV_SIGNAL;
    sig->fd         = -1;
    sig->arg        = arg;
    sig->loop       = NULL;
    sig->is_started = 0;
    sig->callback   = callback;
    sig->signum     = signum;

    sig->is_init    = 1;

    return EV_OK;
}

tEvStatus evsig_start(tEvLoop* loop, tEvSignal* sig)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(sig == NULL, return EV_ERROR, "sig is null");
    check_if(sig->is_init != 1, return EV_ERROR, "sig is not init yet");
    check_if(sig->is_started != 0, return EV_ERROR, "sig has already been started");

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig->signum);

    // Block the signals thet we handle using signalfd(), so they don't
    // cause signal handlers or default signal actions to execute.
    int check = sigprocmask(SIG_BLOCK, &mask, NULL);
    check_if(check < 0, return EV_ERROR, "sigprocmask failed");

    sig->fd = signalfd(-1, &mask, 0);
    check_if(sig->fd <= 0, return EV_ERROR, "signalfd failed");

    // use lock to protect list operation (append, access and remove)
    _enterLock(loop);
    tListStatus list_ret = list_append(&loop->evlist, sig);
    _exitLock(loop);
    check_if(list_ret != LIST_OK, goto _ERROR, "list_append failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = sig,
    };
    check = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, sig->fd, &tmp);
    check_if(check < 0, goto _ERROR, "epoll_ctl add failed");

    sig->loop = loop;
    sig->is_started = 1;

    return EV_OK;

_ERROR:
    if (sig->fd > 0)
    {
        close(sig->fd);
        sig->fd = -1;
    }
    _enterLock(loop);
    list_remove(&loop->evlist, sig);
    _exitLock(loop);
    return EV_ERROR;
}

tEvStatus evsig_stop(tEvLoop* loop, tEvSignal* sig)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(sig == NULL, return EV_ERROR, "sig is null");
    check_if(sig->fd <= 0, return EV_ERROR, "fd = %d invalid", sig->fd);
    check_if(sig->is_init != 1, return EV_ERROR, "sig is not init yet");
    check_if(sig->is_started != 1, return EV_ERROR, "sig is not started");

    _cleanSignalEv(sig);

    _enterLock(loop);
    tListStatus list_ret = list_remove(&loop->evlist, sig);
    _exitLock(loop);
    check_if(list_ret != LIST_OK, return EV_ERROR, "list_remove failed");

    sig->is_started = 0;

    return EV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evtm_init(tEvTimer* tm, tEvTimerCb callback, int time_ms, void* arg, tEvTimerType type)
{
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(callback == NULL, return EV_ERROR, "callback is null");
    check_if(time_ms <= 0, return EV_ERROR, "time_ms = %d invalid", time_ms);

    tm->type       = EV_TIMER;
    tm->fd         = -1;
    tm->arg        = arg;
    tm->loop       = NULL;
    tm->is_started = 0;
    tm->callback   = callback;
    tm->time_ms    = time_ms;
    tm->rest_ms    = -1;
    tm->tm_type    = type;

    tm->is_init    = 1;

    return EV_OK;
}

tEvStatus evtm_start(tEvLoop* loop, tEvTimer* tm)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return EV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return EV_ERROR, "tm is not init yet");
    check_if(tm->is_started != 0, return EV_ERROR, "tm has been already started");

    tm->fd = timerfd_create(CLOCK_MONOTONIC, 0);
    check_if(tm->fd < 0, return EV_ERROR, "timerfd_create failed");

    time_t sec = tm->time_ms / 1000;
    long  nsec = (tm->time_ms % 1000) * 1000 * 1000;

    struct itimerspec timeval = {
        .it_value.tv_sec = sec,
        .it_value.tv_nsec = nsec,
    };

    if (tm->tm_type == EV_TIMER_PERIODIC)
    {
        timeval.it_interval.tv_sec = sec;
        timeval.it_interval.tv_nsec = nsec;
    }

    _enterLock(loop);
    tListStatus list_ret = list_append(&loop->evlist, tm);
    _exitLock(loop);
    check_if(list_ret != LIST_OK, goto _ERROR, "list_append failed");

    int check = timerfd_settime(tm->fd, 0, &timeval, NULL);
    check_if(check < 0, goto _ERROR, "timerfd_settime failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = tm,
    };

    check = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, tm->fd, &tmp);
    check_if(check < 0, goto _ERROR, "epoll_ctl add failed");

    tm->loop = loop;
    tm->is_started = 1;

    return EV_OK;

_ERROR:
    if (tm->fd > 0)
    {
        close(tm->fd);
        tm->fd = -1;
    }
    _enterLock(loop);
    list_remove(&loop->evlist, tm);
    _exitLock(loop);
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

    _cleanTimerEv(tm);

    _enterLock(loop);
    tListStatus list_ret = list_remove(&loop->evlist, tm);
    _exitLock(loop);
    check_if(list_ret != LIST_OK, return EV_ERROR, "list_remove failed");

    tm->is_started = 0;

    return EV_OK;

_ERROR:
    return EV_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus ev_once(tEvLoop* loop, tEvOnceCb callback, void* arg)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(callback == NULL, return EV_ERROR, "callback is null");

    tEvOnce* once = calloc(sizeof(tEvOnce), 1);
    check_if(once == NULL, return EV_ERROR, "calloc failed");

    // init
    once->type     = EV_ONCE;
    once->arg      = arg;
    once->callback = callback;
    once->is_init  = 1;

    // start
    once->fd = eventfd(0, 0);
    check_if(once->fd < 0, goto _ERROR, "eventfd failed");

    int ep_check = -1;
    int wr_check = -1;

    _enterLock(loop);
    tListStatus list_ret = list_append(&loop->evlist, once);
    _exitLock(loop);
    check_if(list_ret != LIST_OK, goto _ERROR, "list_append failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = once,
    };

    ep_check = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, once->fd, &tmp);
    check_if(ep_check < 0, goto _ERROR, "epoll_ctl add failed");

    eventfd_t val = once->fd;
    wr_check = eventfd_write(once->fd, val);
    check_if(wr_check < 0, goto _ERROR, "eventfd_write failed");

    once->loop       = loop;
    once->is_started = 1;

    return EV_OK;

_ERROR:
    if (ep_check == 0) epoll_ctl(loop->epfd, EPOLL_CTL_DEL, once->fd, &tmp);

    if (once) free(once);

    return EV_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

tEvStatus evio_init(tEvIo* io, tEvIoCb callback, int fd, void* arg)
{
    check_if(io == NULL, return EV_ERROR, "io is null");
    check_if(callback == NULL, return EV_ERROR, "callback is null");
    check_if(fd < 0, return EV_ERROR, "fd = %d invalid", fd); // stdin's fd = 0

    io->type       = EV_IO;
    io->fd         = fd;
    io->arg        = arg;
    io->loop       = NULL;
    io->is_started = 0;
    io->callback   = callback;

    io->is_init    = 1;

    return EV_OK;
}

tEvStatus evio_start(tEvLoop* loop, tEvIo* io)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(io == NULL, return EV_ERROR, "io is null");
    check_if(io->is_init != 1, return EV_ERROR, "io is not init yet");
    check_if(io->is_started != 0, return EV_ERROR, "io has been already started");

    _enterLock(loop);
    tListStatus list_ret = list_append(&loop->evlist, io);
    _exitLock(loop);
    check_if(list_ret != LIST_OK, return EV_ERROR, "list_append failed");

    struct epoll_event tmp = {
        .events = EPOLLIN,
        .data.ptr = io,
    };

    int check = epoll_ctl(loop->epfd, EPOLL_CTL_ADD, io->fd, &tmp);
    check_if(check < 0, goto _ERROR, "epoll_ctl add failed");

    io->loop = loop;
    io->is_started = 1;

    return EV_OK;

_ERROR:
    _enterLock(loop);
    list_remove(&loop->evlist, io);
    _exitLock(loop);
    return EV_ERROR;
}

tEvStatus evio_stop(tEvLoop* loop, tEvIo* io)
{
    check_if(loop == NULL, return EV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return EV_ERROR, "loop is not init yet");
    check_if(io == NULL, return EV_ERROR, "io is null");
    check_if(io->is_init != 1, return EV_ERROR, "io is not init yet");
    check_if(io->is_started != 1, return EV_ERROR, "io is not started");

    _cleanIoEv(io);

    _enterLock(loop);
    tListStatus list_ret = list_remove(&loop->evlist, io);
    _exitLock(loop);
    check_if(list_ret != LIST_OK, return EV_ERROR, "list_remove failed");

    io->is_started = 0;

    return EV_OK;
}
