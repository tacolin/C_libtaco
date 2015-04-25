#include "basic.h"
#include "oldevents.h"
#include "list.h"

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct fpoll_rec
{
    int fpd;
    tList data_list; // fpoll_data_t
};

static tList _fpoll_list;
static pthread_mutex_t _fpoll_lock;

static void _cleanFpollData(void* content)
{
    check_if(content == NULL, return, "content is null");
    free(content);
}

static void _cleanFpollRec(void* content)
{
    check_if(content == NULL, return, "content is null");
    struct fpoll_rec* fpoll = (struct fpoll_rec*)content;
    close(fpoll->fpd);
    list_clean(&fpoll->data_list);
    free(content);
    return;
}

static tListBool _findFpoll(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    struct fpoll_rec* fpoll = (struct fpoll_rec*)content;
    int fpd = *((int*)arg);

    return (fpoll->fpd == fpd) ? LIST_TRUE : LIST_FALSE;
}

static tListBool _findFpollData(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    fpoll_data_t* data = (fpoll_data_t*)content;
    int fd = *((int*)arg);
    return (data->fd == fd) ? LIST_TRUE : LIST_FALSE;
}

int fpoll_init(void)
{
    pthread_mutex_init(&_fpoll_lock, NULL);
    list_init(&_fpoll_list, _cleanFpollRec);
    return 0;
}

void fpoll_uninit(void)
{
    pthread_mutex_lock(&_fpoll_lock);
    list_clean(&_fpoll_list);
    pthread_mutex_unlock(&_fpoll_lock);
    pthread_mutex_destroy(&_fpoll_lock);
    return;
}

int fpoll_create(int max)
{
    int fpd = socket(AF_INET, SOCK_DGRAM, 0);
    check_if(fpd < 0, return -1, "socket failed");

    struct fpoll_rec* fpoll = calloc(sizeof(struct fpoll_rec), 1);
    fpoll->fpd = fpd;
    list_init(&fpoll->data_list, _cleanFpollData);

    pthread_mutex_lock(&_fpoll_lock);
    list_append(&_fpoll_list, fpoll);
    pthread_mutex_unlock(&_fpoll_lock);
    return fpd;
}

void fpoll_close(int fpd)
{
    pthread_mutex_lock(&_fpoll_lock);
    void* content = list_find(&_fpoll_list, _findFpoll, &fpd);
    if (content)
    {
        list_remove(&_fpoll_list, content);
        _cleanFpollRec(content);
    }
    pthread_mutex_unlock(&_fpoll_lock);
    return;
}

int fpoll_wait(int fpd, struct fpoll_event *events, int maxevents, int timeout)
{
    check_if(fpd < 0, return -1, "fpd = %d invalid", fpd);
    check_if(events == NULL, return -1, "events is null");
    check_if(maxevents <= 0, return -1, "maxevents = %d invalid", maxevents);

    pthread_mutex_lock(&_fpoll_lock);
    struct fpoll_rec* fpoll = (struct fpoll_rec*)list_find(&_fpoll_list, _findFpoll, &fpd);
    pthread_mutex_unlock(&_fpoll_lock);
    check_if(fpoll == NULL, return -1, "find no fpoll with fpd = %d", fpd);

    fd_set readset;
    struct timeval period;
    void* obj;
    fpoll_data_t* data;
    int ret;

    FD_ZERO(&readset);
    LIST_FOREACH(&fpoll->data_list, obj, data)
    {
        FD_SET(data->fd, &readset);
    }

    if (timeout == -1)
    {
        ret = select(FD_SETSIZE, &readset, NULL, NULL, NULL);
    }
    else
    {
        period.tv_sec = timeout / 1000;
        period.tv_usec = (timeout % 1000) * 1000;
        ret = select(FD_SETSIZE, &readset, NULL, NULL, &period);
    }

    check_if(ret < 0, return -1, "select failed");

    if (ret == 0)
    {
        return 0;
    }

    // ret > 0

    int count = 0;
    LIST_FOREACH(&fpoll->data_list, obj, data)
    {
        if (count > maxevents)
        {
            break;
        }

        if (FD_ISSET(data->fd, &readset))
        {
            events[count].data = *data;
            count++;
        }
    }
    return count;
}

int fpoll_ctl(int fpd, int op, int fd, struct fpoll_event *event)
{
    check_if(fpd < 0, return -1, "fpd = %d invalid", fpd);
    check_if(event == NULL, return -1, "event is null");
    check_if(fd < 0, return -1, "fd = %d invalid", fd);

    pthread_mutex_lock(&_fpoll_lock);
    struct fpoll_rec* fpoll = (struct fpoll_rec*)list_find(&_fpoll_list, _findFpoll, &fpd);
    pthread_mutex_unlock(&_fpoll_lock);
    check_if(fpoll == NULL, return -1, "find no fpoll with fpd = %d", fpd);

    if (op == FPOLL_CTL_ADD)
    {
        fpoll_data_t* data = calloc(sizeof(fpoll_data_t), 1);
        *data = event->data;
        list_append(&fpoll->data_list, data);
    }
    else if (op == FPOLL_CTL_DEL)
    {
        fpoll_data_t* data = (fpoll_data_t*)list_find(&fpoll->data_list, _findFpollData, &fd);
        check_if(data == NULL, return -1, "find no fpoll_data_t with fd = %d", fd);

        list_remove(&fpoll->data_list, data);
        _cleanFpollData(data);
    }
    else
    {
        derror("unknown op = %d", op);
        return -1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

struct tmfd_rec
{
    int tmfd;
    int fdpair[2];
    long rest_ms;
    struct itmspec value;
};

static pthread_mutex_t _tmfd_lock;
static tList _tmfd_running_list;
static tList _tmfd_stopped_list;

static pthread_t _tick_thread;
static int _tick_running = 0;

static tListBool _findTmFd(void* content, void* arg)
{
    check_if(content == NULL, return, "content is null");
    check_if(arg == NULL, return, "arg is null");

    struct tmfd_rec* rec = (struct tmfd_rec*)content;
    int tmfd = *((int*)arg);

    return (rec->tmfd == tmfd) ? LIST_TRUE : LIST_FALSE ;
}

static void _cleanTmFdRec(void* content)
{
    check_if(content == NULL, return, "content is null");
    struct tmfd_rec* rec = (struct tmfd_rec*)content;
    close(rec->fdpair[0]);
    close(rec->fdpair[1]);
    free(content);
    return;
}

static void _addRecToRunningList(struct tmfd_rec* rec)
{
    check_if(rec == NULL, return, "rec is null");

    long total_ms = 0;
    struct tmfd_rec* target;
    void* obj;
    LIST_FOREACH(&_tmfd_running_list, obj, target)
    {
        if ((total_ms + target->rest_ms) > rec->rest_ms)
        {
            break;
        }
        total_ms += target->rest_ms;
    }

    rec->rest_ms -= total_ms;
    if (target)
    {
        list_insertTo(&_tmfd_running_list, target, rec);
        dprint("target->tmfd = %d", target->tmfd);

        tListObj* obj = list_findObj(&_tmfd_running_list, NULL, target);
        for (; obj; obj = list_nextObj(&_tmfd_running_list, obj))
        {
            target = obj->content;
            target->rest_ms -= rec->rest_ms;
        }
    }
    else
    {
        // add to tail
        list_append(&_tmfd_running_list, rec);
    }
    return;
}

static void* _tickRoutine(void* arg)
{
    struct timeval period;
    struct tmfd_rec* rec;
    tListObj* obj;
    uint64_t dummy;
    ssize_t  dummy_size = sizeof(dummy);

    while (_tick_running)
    {
        period.tv_sec = TM_TICK_MS / 1000;
        period.tv_usec = (TM_TICK_MS % 1000) * 1000;

        select(0, 0, 0, 0, &period);

        pthread_mutex_lock(&_tmfd_lock);
        while (rec = list_head(&_tmfd_running_list))
        {
            rec->rest_ms -= TM_TICK_MS;
            if (rec->rest_ms > 0)
            {
                break;
            }

            dummy = 1;
            write(rec->fdpair[1], &dummy, dummy_size);

            list_remove(&_tmfd_running_list, rec);

            if ((rec->value.it_interval.tv_sec > 0) || (rec->value.it_interval.tv_nsec > 0))
            {
                // periodic timer
                rec->rest_ms = rec->value.it_interval.tv_sec * 1000;
                rec->rest_ms += rec->value.it_interval.tv_nsec / (1000*1000);
                _addRecToRunningList(rec);
            }
            else
            {
                rec->rest_ms = 0;
                list_append(&_tmfd_stopped_list, rec);
            }
        }
        pthread_mutex_unlock(&_tmfd_lock);
    }

    pthread_exit(NULL);
    return NULL;
}

int tmfd_init(void)
{
    pthread_mutex_init(&_tmfd_lock, NULL);
    list_init(&_tmfd_running_list, _cleanTmFdRec);
    list_init(&_tmfd_stopped_list, _cleanTmFdRec);

    _tick_running = 1;
    pthread_create(&_tick_thread, NULL, _tickRoutine, NULL);
    return 0;
}

void tmfd_uninit(void)
{
    _tick_running = 0;
    pthread_join(_tick_thread, NULL);

    pthread_mutex_lock(&_tmfd_lock);
    list_clean(&_tmfd_running_list);
    list_clean(&_tmfd_stopped_list);
    pthread_mutex_unlock(&_tmfd_lock);
    pthread_mutex_destroy(&_tmfd_lock);
    return;
}

int tmfd_create(int clockid, int flags)
{
    check_if(clockid != TM_CLOCK_MONOTONIC, return -1, "clockid shall be TM_CLOCK_MONOTONIC");

    int fdpair[2];

    int chk = socketpair(PF_UNIX, SOCK_DGRAM, 0, fdpair);
    check_if(chk < 0, return -1, "socketpair failed");

    struct tmfd_rec* rec = calloc(sizeof(struct tmfd_rec), 1);
    rec->tmfd = fdpair[0];
    rec->fdpair[0] = fdpair[0];
    rec->fdpair[1] = fdpair[1];
    rec->rest_ms = 0;

    list_append(&_tmfd_stopped_list, rec);
    dprint("tmfd = %d, ok", rec->tmfd);
    return fdpair[0];
}

int tmfd_settime(int tmfd, int flags, const struct itmspec *new_value, struct itmspec *old_value)
{
    check_if(tmfd < 0, return -1, "tmfd = %d invalid", tmfd);
    check_if(new_value == NULL, return -1, "new_value is null");

    pthread_mutex_lock(&_tmfd_lock);
    struct tmfd_rec* rec;
    tList* origin_list;

    // rec in stopped list ?
    origin_list = &_tmfd_stopped_list;
    rec = list_find(origin_list, _findTmFd, &tmfd);
    if (rec == NULL)
    {
        origin_list = &_tmfd_running_list;
        // rec in running list ?
        rec = list_find(origin_list, _findTmFd, &tmfd);
        check_if(rec == NULL, goto _ERROR, "find no tmfd_rec with tmfd = %d", tmfd);
    }
    list_remove(origin_list, rec);

    rec->value   = *new_value;

    // if new value is 0, add to stopped list
    long sum = new_value->it_value.tv_sec + new_value->it_value.tv_nsec + new_value->it_interval.tv_sec + new_value->it_interval.tv_nsec;
    if (sum == 0)
    {
        list_append(&_tmfd_stopped_list, rec);
        goto _END;
    }

    rec->rest_ms = new_value->it_value.tv_sec * 1000 + new_value->it_value.tv_nsec / (1000*1000);

    _addRecToRunningList(rec);

_END:
    pthread_mutex_unlock(&_tmfd_lock);
    return 0;

_ERROR:
    pthread_mutex_unlock(&_tmfd_lock);
    return -1;
}

int tmfd_gettime(int tmfd, struct itmspec *old_value)
{
    check_if(tmfd < 0, return -1, "tmfd = %d invalid", tmfd);
    check_if(old_value == NULL, return -1, "old_value is null");

    pthread_mutex_lock(&_tmfd_lock);
    struct tmfd_rec* rec;

    rec = list_find(&_tmfd_running_list, _findTmFd, &tmfd);
    if (rec == NULL)
    {
        rec = list_find(&_tmfd_stopped_list, _findTmFd, &tmfd);
        check_if(rec == NULL, goto _ERROR, "find no tmfd_rec with tmfd = %d", tmfd);

        memset(old_value, 0, sizeof(struct itmspec));
        goto _END;
    }

    old_value->it_value.tv_sec = rec->rest_ms / 1000;
    old_value->it_value.tv_nsec = (rec->rest_ms % 1000) * 1000 * 1000;
    old_value->it_interval = rec->value.it_interval;

_END:
    pthread_mutex_unlock(&_tmfd_lock);
    return 0;

_ERROR:
    pthread_mutex_unlock(&_tmfd_lock);
    return -1;
}

void tmfd_close(int tmfd)
{
    pthread_mutex_lock(&_tmfd_lock);

    struct tmfd_rec* rec;
    rec = list_find(&_tmfd_running_list, _findTmFd, &tmfd);
    if (rec)
    {
        list_remove(&_tmfd_running_list, rec);
        _cleanTmFdRec(rec);
    }
    else
    {
        rec = list_find(&_tmfd_stopped_list, _findTmFd, &tmfd);
        if (rec)
        {
            list_remove(&_tmfd_stopped_list, rec);
            _cleanTmFdRec(rec);
        }
    }

    pthread_mutex_unlock(&_tmfd_lock);
    return;
}
