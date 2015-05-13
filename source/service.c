#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "service.h"
#include "thread.h"

#define dtrace() do { struct timeval _now = {}; gettimeofday(&_now, NULL); fprintf(stdout, "(%3lds,%3ldms), tid = %lu, line %4d, %s()\n", _now.tv_sec % 1000, _now.tv_usec/1000, (unsigned long)pthread_self(), __LINE__, __func__); } while(0)
#define dprint(a, b...) fprintf(stdout, "%s(): "a"\n", __func__, ##b)
#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static struct map _services;
static bool _running = false;

static void _clean_watcher(void* input)
{
    if (input)
    {
        struct watcher* w = (struct watcher*)input;
        close(w->fd);
        free(input);
    }
}

static void _clean_service(void* input)
{
    if (input)
    {
        struct service* s = (struct service*)input;
        fqueue_release(s->mq);
        map_uninit(&s->watchers);
        free(s);
    }
}

int service_system_init(void)
{
    return map_init(&_services, _clean_service);
}

void service_system_uninit(void)
{
    if (_running) service_system_break();
    map_uninit(&_services);
}

static void _stop_watching(serviceid sid, void* db, int fd, void* arg)
{
    struct service* s = (struct service*)arg;
    if (fd == s->stopfd)
    {
        eventfd_t val;
        eventfd_read(fd, &val);
        s->watching = false;
    }
    return;
}

static void _dequeue(serviceid sid, void* db, int fd, void* arg)
{
    struct service* s = (struct service*)arg;
    if (fd == s->qfd)
    {
        eventfd_t val;
        eventfd_read(fd, &val);

        struct service_msg *qmsg;
        ret_t ret;
        FQUEUE_FOREACH(s->mq, qmsg)
        {
            if (s->handlemsg)
            {
                ret = s->handlemsg(s->id, s->db, qmsg->session, qmsg->src, qmsg->msg, qmsg->msglen);
                if (ret != RET_DONTFREE)
                {
                    free(qmsg->msg);
                }
                free(qmsg);
            }
        }
    }
    return;
}

int service_send(serviceid src, serviceid dst, tag_t tag, int session, void* msg, int msglen)
{
    CHECK_IF(src == INVALID_ID, return -1, "src is INVALID_ID");
    CHECK_IF(dst == INVALID_ID, return -1, "dst is INVALID_ID");
    CHECK_IF(msg == NULL, return -1, "msg is null");

    struct service* s = map_grab(&_services, dst);
    CHECK_IF(s == NULL, return -1, "map_grab service with id = %d failed", dst);

    struct service_msg* qmsg = calloc(sizeof(struct service_msg), 1);
    qmsg->session = session;
    qmsg->src     = src;
    qmsg->msglen  = msglen;

    if (tag == TAG_DONTCOPY)
    {
        qmsg->msg = msg;
    }
    else
    {
        qmsg->msg = malloc(msglen);
        memcpy(qmsg->msg, msg, msglen);
    }
    fqueue_push(s->mq, qmsg);
    eventfd_t val = 1;
    eventfd_write(s->qfd, val);

    map_release(&_services, dst);
    return msglen;
}

static void _timeout_callback(serviceid sid, void* db, int fd, void* arg)
{
    uint64_t val;
    ssize_t  sz = sizeof(val);
    read(fd, &val, sz);

    struct watcher* w = (struct watcher*)arg;
    w->tmcallback(sid, db, w->arg);
    if (w->interval_ms <= 0)
    {
        service_stop_timer(w->sid, w->id);
    }
}

watchid service_start_timer(serviceid sid, int time_ms, int interval_ms, void (*callback)(serviceid sid, void* db, void* arg), void* arg)
{
    CHECK_IF(sid == INVALID_ID, return INVALID_ID, "sid is INVALID_ID");
    CHECK_IF(time_ms < 0, return INVALID_ID, "time_ms = %d invalid", time_ms);
    CHECK_IF(interval_ms < 0, return INVALID_ID, "interval_ms = %d invalid", interval_ms);
    CHECK_IF((time_ms + interval_ms == 0), return INVALID_ID, "time_ms = interval_ms = 0 invalid");
    CHECK_IF(callback == NULL, return INVALID_ID, "callback is null");

    struct service* s = map_grab(&_services, sid);
    CHECK_IF(s == NULL, return INVALID_ID, "map_grab service by id = %d failed", sid);

    int fd = timerfd_create(CLOCK_MONOTONIC, 0);

    struct watcher* w = calloc(sizeof(struct watcher), 1);
    w->fd          = fd;
    w->callback    = _timeout_callback;
    w->arg         = w;
    w->id          = map_new(&s->watchers, w);

    w->tmcallback  = callback;
    w->time_ms     = time_ms;
    w->interval_ms = interval_ms;
    w->sid         = sid;

    struct epoll_event tmp = {.events = EPOLLIN, .data.u64 = w->id};
    epoll_ctl(s->epfd, EPOLL_CTL_ADD, fd, &tmp);

    struct itimerspec timeval = {
        .it_value.tv_sec     = time_ms / 1000,
        .it_value.tv_nsec    = (time_ms % 1000) * 1000 * 1000,
        .it_interval.tv_sec  = interval_ms / 1000,
        .it_interval.tv_nsec = (interval_ms % 1000) * 1000 * 1000
    };
    timerfd_settime(fd, 0, &timeval, NULL);
    dtrace();

    map_release(&_services, sid);
    return w->id;
}

void service_stop_timer(serviceid sid, watchid wid)
{
    service_unwatch(sid, wid);
}

watchid service_watch(serviceid sid, int fd, void (*callback)(serviceid sid, void* db, int fd, void* arg), void* arg)
{
    CHECK_IF(sid == INVALID_ID, return INVALID_ID, "sid is INVALID_ID");
    CHECK_IF(fd < 0, return INVALID_ID, "fd = %d invalid", fd);
    CHECK_IF(callback == NULL, return INVALID_ID, "callback is null");

    struct service* s = map_grab(&_services, sid);
    CHECK_IF(s == NULL, return INVALID_ID, "map_grab service by id = %d failed", sid);

    struct watcher* w = calloc(sizeof(struct watcher), 1);
    w->fd       = fd;
    w->callback = callback;
    w->arg      = arg;
    w->id       = map_new(&s->watchers, w);

    struct epoll_event tmp = {.events = EPOLLIN, .data.u64 = w->id};
    epoll_ctl(s->epfd, EPOLL_CTL_ADD, fd, &tmp);

    map_release(&_services, sid);
    return w->id;
}

void service_unwatch(serviceid sid, watchid wid)
{
    CHECK_IF(sid == INVALID_ID, return, "sid is INVALID_ID");
    CHECK_IF(wid == INVALID_ID, return, "wid is INVALID_ID");

    struct service* s = map_grab(&_services, sid);
    CHECK_IF(s == NULL, return, "map_grab service by id = %d failed", sid);

    struct watcher* w = map_grab(&s->watchers, wid);
    if (w)
    {
        struct epoll_event tmp = {};
        epoll_ctl(s->epfd, EPOLL_CTL_DEL, w->fd, &tmp);
        map_release(&s->watchers, wid);
    }
    map_release(&s->watchers, wid); // free map_slot

    map_release(&_services, sid);
    return;
}

static void _clean_qmsg(void* input)
{
    struct service_msg* qmsg = (struct service_msg*)input;
    if (input)
    {
        if (qmsg->msg) free(qmsg->msg);
        free(input);
    }
}

serviceid service_create(char* name, void* db, service_cb handlemsg, void (*init)(serviceid sid, void* db), void (*uninit)(serviceid sid, void* db))
{
    CHECK_IF(name == NULL, return INVALID_ID, "name is null");
    CHECK_IF(handlemsg == NULL, return INVALID_ID, "handlemsg is null");

    struct service *s = calloc(sizeof(struct service), 1);
    snprintf(s->name, SERVICE_NAME_SIZE+1, "%s", name);
    s->db        = db;
    s->handlemsg = handlemsg;
    s->init      = init;
    s->uninit    = uninit;
    s->epfd      = epoll_create(MAX_WATCH_NUM);
    s->qfd       = eventfd(0, 0);
    s->stopfd    = eventfd(0, 0);
    map_init(&s->watchers, _clean_watcher);
    s->watching  = false;
    s->id        = map_new(&_services, s);
    s->mq        = fqueue_create(_clean_qmsg);

    service_watch(s->id, s->qfd, _dequeue, s);
    service_watch(s->id, s->stopfd, _stop_watching, s);
    return s->id;
}

void _service_routine(void* arg)
{
    serviceid id = (serviceid)(intptr_t)arg;
    struct service* s = map_grab(&_services, id);
    CHECK_IF(s == NULL, return, "map_grab service id = %d falied", id);

    struct epoll_event evbuf[MAX_WATCH_NUM] = {0};
    int num;
    int i;
    watchid wid;
    struct watcher* w;

    s->watching = true;
    while (s->watching)
    {
        num = epoll_wait(s->epfd, evbuf, MAX_WATCH_NUM, -1);
        if (num <= 0) break;

        for (i=0; i<num && s->watching; i++)
        {
            wid = (watchid)(evbuf[i].data.u64);
            w   = map_grab(&s->watchers, wid);
            if (w)
            {
                w->callback(s->id, s->db, w->fd, w->arg);
                if (map_release(&s->watchers, wid))
                {
                    free(w);
                }
            }
        }
    }
    map_release(&_services, id);
    return;
}

void service_system_run(void)
{
    serviceid idbuf[MAX_SERVICE_NUM] = {0};
    int num = map_list(&_services, idbuf, MAX_SERVICE_NUM);

    struct thread t[num];
    int i;
    for (i=0; i<num; i++)
    {
        t[i].arg = (void*)(intptr_t)idbuf[i];
        t[i].func = _service_routine;

        struct service* s = map_grab(&_services, idbuf[i]);
        CHECK_IF(s == NULL, return, "map_grab service with id = %d failed", idbuf[i]);

        if (s->init) s->init(idbuf[i], s->db);

        map_release(&_services, idbuf[i]);
    }

    thread_join(t, num);

    for (i=0; i<num; i++)
    {
        struct service* s = map_grab(&_services, idbuf[i]);
        CHECK_IF(s == NULL, return, "map_grab service with id = %d failed", idbuf[i]);

        if (s->uninit) s->uninit(idbuf[i], s->db);

        map_release(&_services, idbuf[i]);
    }
    return;
}

void service_system_break(void)
{
    serviceid idbuf[MAX_SERVICE_NUM] = {0};
    int num = map_list(&_services, idbuf, MAX_SERVICE_NUM);
    int i;
    eventfd_t val = 1;
    for (i=0; i<num; i++)
    {
        struct service* s = map_grab(&_services, idbuf[i]);
        if (s)
        {
            eventfd_write(s->stopfd, val);
            map_release(&_services, idbuf[i]);
        }
    }
    return;
}

serviceid service_getid(char* name)
{
    CHECK_IF(name == NULL, return INVALID_ID, "name is null");
    serviceid idbuf[MAX_SERVICE_NUM] = {0};
    int num = map_list(&_services, idbuf, MAX_SERVICE_NUM);
    int i;
    int cmp;
    for (i=0; i<num; i++)
    {
        struct service* s = map_grab(&_services, idbuf[i]);
        cmp = strcmp(s->name, name);
        map_release(&_services, idbuf[i]);

        if (cmp == 0) return idbuf[i];
    }
    return INVALID_ID;
}
