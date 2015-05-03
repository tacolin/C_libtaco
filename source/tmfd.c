#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "basic.h"
#include "list.h"
#include "tmfd.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct tTmfdRec
{
    int tmfd;
    int fdpair[2];

    struct timeval expire;
    struct itmfdspec value;

} tTmfdRec;

////////////////////////////////////////////////////////////////////////////////

static pthread_mutex_t _tmfd_lock;
static tList _running_list;
static tList _stopped_list;

static pthread_t _tick_thread;
static int _is_running = 0;

////////////////////////////////////////////////////////////////////////////////

static int _findTmFd(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    tTmfdRec* rec = (tTmfdRec*)content;
    int tmfd = *((int*)arg);
    return (rec->tmfd == tmfd) ? LIST_TRUE : LIST_FALSE ;
}

static void _cleanTmFdRec(void* content)
{
    check_if(content == NULL, return, "content is null");
    tTmfdRec* rec = (tTmfdRec*)content;
    close(rec->fdpair[0]);
    close(rec->fdpair[1]);
    free(content);
    return;
}

static void _addRecToRunningList(tTmfdRec* rec)
{
    check_if(rec == NULL, return, "rec is null");

    tTmfdRec* target;
    void* obj;
    LIST_FOREACH(&_running_list, obj, target)
    {
        if (timercmp(&(target->expire), &(rec->expire), >))
        {
            break;
        }
    }

    if (target)
    {
        list_insertTo(&_running_list, target, rec);
    }
    else
    {
        // add to tail
        list_append(&_running_list, rec);
    }
    return;
}

static void* _tickRoutine(void* arg)
{
    struct timeval period;
    tTmfdRec* rec;
    uint64_t dummy;
    ssize_t  dummy_size = sizeof(dummy);
    struct timeval curr;

    while (_is_running)
    {
        period.tv_sec  = TMFD_TICK_MS / 1000;
        period.tv_usec = (TMFD_TICK_MS % 1000) * 1000;

        select(0, 0, 0, 0, &period);

        pthread_mutex_lock(&_tmfd_lock);
        while ((rec = list_head(&_running_list)) != NULL)
        {
            gettimeofday(&curr, NULL);

            if (timercmp(&curr, &(rec->expire), <))
            {
                break;
            }

            dprint("curr = %3ld sec, %3ld ms", curr.tv_sec % 1000, curr.tv_usec / 1000);
            dprint("rec->expire = %3ld sec, %3ld ms", rec->expire.tv_sec % 1000, rec->expire.tv_usec / 1000);

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
                _addRecToRunningList(rec);
            }
            else
            {
                timerclear(&(rec->expire));
                list_append(&_stopped_list, rec);
            }
        }
        pthread_mutex_unlock(&_tmfd_lock);
    }

    pthread_exit(NULL);
    return NULL;
}

int tmfd_system_init(void)
{
    pthread_mutex_init(&_tmfd_lock, NULL);
    list_init(&_running_list, _cleanTmFdRec);
    list_init(&_stopped_list, _cleanTmFdRec);

    _is_running = 1;
    pthread_create(&_tick_thread, NULL, _tickRoutine, NULL);

    return TMFD_OK;
}

void tmfd_system_uninit(void)
{
    _is_running = 0;
    pthread_join(_tick_thread, NULL);

    pthread_mutex_lock(&_tmfd_lock);
    list_clean(&_running_list);
    list_clean(&_stopped_list);
    pthread_mutex_unlock(&_tmfd_lock);
    pthread_mutex_destroy(&_tmfd_lock);

    return;
}

int tmfd_create(int clockid, int flags)
{
    check_if(clockid != TMFD_CLOCK_MONOTONIC, return TMFD_FAIL, "clockid shall be TMFD_CLOCK_MONOTONIC");

    int fdpair[2];
    int chk = socketpair(PF_UNIX, SOCK_DGRAM, 0, fdpair);
    check_if(chk < 0, return -1, "socketpair failed");

    tTmfdRec* rec  = calloc(sizeof(tTmfdRec), 1);
    rec->tmfd      = fdpair[0];
    rec->fdpair[0] = fdpair[0];
    rec->fdpair[1] = fdpair[1];

    list_append(&_stopped_list, rec);
    return fdpair[0];
}

int tmfd_settime(int tmfd, int flags, const struct itmfdspec *new_value, struct itmfdspec *old_value)
{
    check_if(tmfd < 0, return TMFD_FAIL, "tmfd = %d invalid", tmfd);
    check_if(new_value == NULL, return TMFD_FAIL, "new_value is null");

    pthread_mutex_lock(&_tmfd_lock);

    tTmfdRec* rec;
    if (rec = list_find(&_stopped_list, _findTmFd, &tmfd))
    {
        list_remove(&_stopped_list, rec);
    }
    else if (rec == list_find(&_running_list, _findTmFd, &tmfd))
    {
        list_remove(&_running_list, rec);
    }
    check_if(rec == NULL, goto _ERROR, "find no tmfd_rec with tmfd = %d", tmfd);

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
    gettimeofday(&curr, NULL);

    new_time_val.tv_sec = new_value->it_value.tv_sec;
    new_time_val.tv_usec = new_value->it_value.tv_nsec / 1000;

    timeradd(&curr, &new_time_val, &(rec->expire));
    _addRecToRunningList(rec);

_END:
    pthread_mutex_unlock(&_tmfd_lock);
    return TMFD_OK;

_ERROR:
    pthread_mutex_unlock(&_tmfd_lock);
    return TMFD_FAIL;
}

int tmfd_gettime(int tmfd, struct itmfdspec *old_value)
{
    check_if(tmfd < 0, return TMFD_FAIL, "tmfd = %d invalid", tmfd);
    check_if(old_value == NULL, return TMFD_FAIL, "old_value is null");

    pthread_mutex_lock(&_tmfd_lock);

    tTmfdRec* rec = list_find(&_running_list, _findTmFd, &tmfd);
    if (rec == NULL)
    {
        rec = list_find(&_stopped_list, _findTmFd, &tmfd);
        check_if(rec == NULL, goto _ERROR, "find no tmfd_rec with tmfd = %d", tmfd);

        memset(old_value, 0, sizeof(struct itmfdspec));
        goto _END;
    }

    struct timeval curr, rest;
    gettimeofday(&curr, NULL);

    timersub(&(rec->expire), &curr, &rest);

    old_value->it_value.tv_sec  = rest.tv_sec;
    old_value->it_value.tv_nsec = rest.tv_usec * 1000;
    old_value->it_interval      = rec->value.it_interval;

_END:
    pthread_mutex_unlock(&_tmfd_lock);
    return TMFD_OK;

_ERROR:
    pthread_mutex_unlock(&_tmfd_lock);
    return TMFD_FAIL;
}

void tmfd_close(int tmfd)
{
    pthread_mutex_lock(&_tmfd_lock);

    tTmfdRec* rec = list_find(&_running_list, _findTmFd, &tmfd);
    if (rec)
    {
        list_remove(&_running_list, rec);
        _cleanTmFdRec(rec);
    }
    else
    {
        rec = list_find(&_stopped_list, _findTmFd, &tmfd);
        if (rec)
        {
            list_remove(&_stopped_list, rec);
            _cleanTmFdRec(rec);
        }
    }

    pthread_mutex_unlock(&_tmfd_lock);
    return;
}
