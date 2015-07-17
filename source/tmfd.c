#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "list.h"
#include "tmfd.h"

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#define LOCK() atom_spinlock(&_tmfd_lock)
#define UNLOCK() atom_spinunlock(&_tmfd_lock)

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

struct tmfd_rec
{
    int tmfd;
    int fdpair[2];

    struct timeval expire;
    struct itmfdspec value;

};

static int _tmfd_lock = 0;
static struct list _running_list;
static struct list _stopped_list;

static pthread_t _tick_thread;
static int _is_running = 0;

static int _get_time_of_day(struct timeval* timeval, void* arg)
{
    struct timespec spec = {};
    int ret = clock_gettime(CLOCK_MONOTONIC, &spec);
    if (ret == -1)
    {
        return ret;
    }
    timeval->tv_sec  = spec.tv_sec;
    timeval->tv_usec = spec.tv_nsec / 1000;
    return ret;
}

static int _find_tmfd_rec(void* data, void* arg)
{
    CHECK_IF(data == NULL, return 0, "data is null");
    CHECK_IF(arg == NULL, return 0, "arg is null");

    struct tmfd_rec* rec = (struct tmfd_rec*)data;
    int tmfd = *((int*)arg);
    return (rec->tmfd == tmfd) ? 1 : 0 ;
}

static void _clean_tmfd_rec(void* data)
{
    CHECK_IF(data == NULL, return, "data is null");

    struct tmfd_rec* rec = (struct tmfd_rec*)data;
    close(rec->fdpair[0]);
    close(rec->fdpair[1]);
    free(data);
    return;
}

static void _add_rec_to_running_list(struct tmfd_rec* rec)
{
    CHECK_IF(rec == NULL, return, "rec is null");

    struct tmfd_rec* target;
    void* obj;
    LIST_FOREACH(&_running_list, obj, target)
    {
        if (timercmp(&(target->expire), &(rec->expire), >)) break;
    }

    if (target)
    {
        list_insert_before(&_running_list, target, rec);
    }
    else
    {
        // add to tail
        list_append(&_running_list, rec);
    }
    return;
}

static void* _tick_routine(void* arg)
{
    struct timeval period;
    struct tmfd_rec* rec;
    uint64_t dummy;
    ssize_t  dummy_size = sizeof(dummy);
    struct timeval curr;

    while (_is_running)
    {
        period.tv_sec  = TMFD_TICK_MS / 1000;
        period.tv_usec = (TMFD_TICK_MS % 1000) * 1000;

        select(0, 0, 0, 0, &period);

        LOCK();
        while ((rec = list_head(&_running_list)) != NULL)
        {
            _get_time_of_day(&curr, NULL);

            if (timercmp(&curr, &(rec->expire), <)) break;


            dummy = 1;
            write(rec->fdpair[1], &dummy, dummy_size);

            list_remove(&_running_list, rec);

            if ((rec->value.it_interval.tv_sec > 0) || (rec->value.it_interval.tv_nsec > 0))
            {
                // periodic timer
                struct timeval time_val;
                time_val.tv_sec = rec->value.it_interval.tv_sec;
                time_val.tv_usec = rec->value.it_interval.tv_nsec / 1000;
                timeradd(&(rec->expire), &time_val, &(rec->expire));
                _add_rec_to_running_list(rec);
            }
            else
            {
                timerclear(&(rec->expire));
                list_append(&_stopped_list, rec);
            }
        }
        UNLOCK();
    }
    pthread_exit(NULL);
    return NULL;
}

int tmfd_system_init(void)
{
    pthread_mutex_init(&_tmfd_lock, NULL);
    list_init(&_running_list, _clean_tmfd_rec);
    list_init(&_stopped_list, _clean_tmfd_rec);

    _is_running = 1;
    pthread_create(&_tick_thread, NULL, _tick_routine, NULL);
    return TMFD_OK;
}

void tmfd_system_uninit(void)
{
    _is_running = 0;
    pthread_join(_tick_thread, NULL);

    LOCK();
    list_clean(&_running_list);
    list_clean(&_stopped_list);
    UNLOCK();
    return;
}

int tmfd_create(int clockid, int flags)
{
    CHECK_IF(clockid != TMFD_CLOCK_MONOTONIC, return TMFD_FAIL, "clockid shall be TMFD_CLOCK_MONOTONIC");

    int fdpair[2];
    int chk = socketpair(PF_UNIX, SOCK_DGRAM, 0, fdpair);
    CHECK_IF(chk < 0, return -1, "socketpair failed");

    struct tmfd_rec* rec  = calloc(sizeof(struct tmfd_rec), 1);
    rec->tmfd      = fdpair[0];
    rec->fdpair[0] = fdpair[0];
    rec->fdpair[1] = fdpair[1];

    list_append(&_stopped_list, rec);
    return fdpair[0];
}

int tmfd_settime(int tmfd, int flags, const struct itmfdspec *new_value, struct itmfdspec *old_value)
{
    CHECK_IF(tmfd < 0, return TMFD_FAIL, "tmfd = %d invalid", tmfd);
    CHECK_IF(new_value == NULL, return TMFD_FAIL, "new_value is null");

    LOCK();

    struct tmfd_rec* rec;
    rec = list_find(&_stopped_list, _find_tmfd_rec, &tmfd);
    if (rec)
    {
        list_remove(&_stopped_list, rec);
    }
    else
    {
        rec = list_find(&_running_list, _find_tmfd_rec, &tmfd);
        if (rec)
        {
            list_remove(&_running_list, rec);
        }
    }
    CHECK_IF(rec == NULL, goto _ERROR, "find no tmfd_rec with tmfd = %d", tmfd);

    rec->value = *new_value;

    // if new value is 0, add to stopped list
    long sum = new_value->it_value.tv_sec + new_value->it_value.tv_nsec + new_value->it_interval.tv_sec + new_value->it_interval.tv_nsec;
    if (sum == 0)
    {
        timerclear(&(rec->expire));
        list_append(&_stopped_list, rec);
        goto _END;
    }

    struct timeval curr, new_time_val;
    _get_time_of_day(&curr, NULL);

    new_time_val.tv_sec  = new_value->it_value.tv_sec;
    new_time_val.tv_usec = new_value->it_value.tv_nsec / 1000;

    timeradd(&curr, &new_time_val, &(rec->expire));
    _add_rec_to_running_list(rec);

_END:
    UNLOCK();
    return TMFD_OK;

_ERROR:
    UNLOCK();
    return TMFD_FAIL;
}

int tmfd_gettime(int tmfd, struct itmfdspec *old_value)
{
    CHECK_IF(tmfd < 0, return TMFD_FAIL, "tmfd = %d invalid", tmfd);
    CHECK_IF(old_value == NULL, return TMFD_FAIL, "old_value is null");

    LOCK();

    struct tmfd_rec* rec = list_find(&_running_list, _find_tmfd_rec, &tmfd);
    if (rec == NULL)
    {
        rec = list_find(&_stopped_list, _find_tmfd_rec, &tmfd);
        CHECK_IF(rec == NULL, goto _ERROR, "find no tmfd_rec with tmfd = %d", tmfd);

        memset(old_value, 0, sizeof(struct itmfdspec));
        goto _END;
    }

    struct timeval curr, rest;
    _get_time_of_day(&curr, NULL);

    timersub(&(rec->expire), &curr, &rest);

    old_value->it_value.tv_sec  = rest.tv_sec;
    old_value->it_value.tv_nsec = rest.tv_usec * 1000;
    old_value->it_interval      = rec->value.it_interval;

_END:
    UNLOCK();
    return TMFD_OK;

_ERROR:
    UNLOCK();
    return TMFD_FAIL;
}

void tmfd_close(int tmfd)
{
    LOCK();
    struct tmfd_rec* rec = list_find(&_running_list, _find_tmfd_rec, &tmfd);
    if (rec)
    {
        list_remove(&_running_list, rec);
        _clean_tmfd_rec(rec);
    }
    else
    {
        rec = list_find(&_stopped_list, _find_tmfd_rec, &tmfd);
        if (rec)
        {
            list_remove(&_stopped_list, rec);
            _clean_tmfd_rec(rec);
        }
    }
    UNLOCK();
    return;
}
