#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "basic.h"
#include "fpoll.h"
#include "list.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct tFpollRec
{
    int fpd;

    tList data_list; // fpoll_data_t
    pthread_mutex_t data_lock;

} tFpollRec;

////////////////////////////////////////////////////////////////////////////////

static tList _fpoll_list;
static pthread_mutex_t _fpoll_lock;

////////////////////////////////////////////////////////////////////////////////

static void _cleanFpollData(void* content)
{
    check_if(content == NULL, return, "content is null");
    free(content);
}

static void _cleanFpollRec(void* content)
{
    check_if(content == NULL, return, "content is null");

    tFpollRec* fpoll = (tFpollRec*)content;
    close(fpoll->fpd);

    pthread_mutex_lock(&fpoll->data_lock);
    list_clean(&fpoll->data_list);
    pthread_mutex_unlock(&fpoll->data_lock);
    pthread_mutex_destroy(&fpoll->data_lock);

    free(content);
    return;
}

static int _findFpoll(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    tFpollRec* fpoll = (tFpollRec*)content;
    int fpd = *((int*)arg);
    return (fpoll->fpd == fpd) ? LIST_TRUE : LIST_FALSE;
}

static int _findFpollData(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    fpoll_data_t* data = (fpoll_data_t*)content;
    int fd = *((int*)arg);
    return (data->fd == fd) ? LIST_TRUE : LIST_FALSE;
}

////////////////////////////////////////////////////////////////////////////////

int fpoll_system_init(void)
{
    pthread_mutex_init(&_fpoll_lock, NULL);
    list_init(&_fpoll_list, _cleanFpollRec);
    return 0;
}

void fpoll_system_uninit(void)
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

    tFpollRec* fpoll = calloc(sizeof(tFpollRec), 1);
    fpoll->fpd = fpd;
    list_init(&fpoll->data_list, _cleanFpollData);
    pthread_mutex_init(&fpoll->data_lock, NULL);

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
    tFpollRec* fpoll = (tFpollRec*)list_find(&_fpoll_list, _findFpoll, &fpd);
    pthread_mutex_unlock(&_fpoll_lock);
    check_if(fpoll == NULL, return -1, "find no fpoll with fpd = %d", fpd);

    fd_set readset;
    struct timeval period;
    void* obj;
    fpoll_data_t* data;
    int ret;

    FD_ZERO(&readset);

    pthread_mutex_lock(&fpoll->data_lock);
    LIST_FOREACH(&fpoll->data_list, obj, data)
    {
        FD_SET(data->fd, &readset);
    }
    pthread_mutex_unlock(&fpoll->data_lock);

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
    pthread_mutex_lock(&fpoll->data_lock);
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
    pthread_mutex_unlock(&fpoll->data_lock);

    return count;
}

int fpoll_ctl(int fpd, int op, int fd, struct fpoll_event *event)
{
    check_if(fpd < 0, return FPOLL_FAIL, "fpd = %d invalid", fpd);
    check_if(event == NULL, return FPOLL_FAIL, "event is null");
    check_if(fd < 0, return FPOLL_FAIL, "fd = %d invalid", fd);

    pthread_mutex_lock(&_fpoll_lock);
    tFpollRec* fpoll = (tFpollRec*)list_find(&_fpoll_list, _findFpoll, &fpd);
    pthread_mutex_unlock(&_fpoll_lock);
    check_if(fpoll == NULL, return FPOLL_FAIL, "find no fpoll with fpd = %d", fpd);

    if (op == FPOLL_CTL_ADD)
    {
        fpoll_data_t* data = calloc(sizeof(fpoll_data_t), 1);
        *data = event->data;

        data->fd = fd;

        pthread_mutex_lock(&fpoll->data_lock);
        list_append(&fpoll->data_list, data);
        pthread_mutex_unlock(&fpoll->data_lock);
    }
    else if (op == FPOLL_CTL_DEL)
    {
        pthread_mutex_lock(&fpoll->data_lock);
        fpoll_data_t* data = (fpoll_data_t*)list_find(&fpoll->data_list, _findFpollData, &fd);
        pthread_mutex_unlock(&fpoll->data_lock);

        check_if(data == NULL, return FPOLL_FAIL, "find no fpoll_data_t with fd = %d", fd);

        pthread_mutex_lock(&fpoll->data_lock);
        list_remove(&fpoll->data_list, data);
        pthread_mutex_unlock(&fpoll->data_lock);

        _cleanFpollData(data);
    }
    else
    {
        derror("unknown op = %d", op);
        return FPOLL_FAIL;
    }
    return FPOLL_OK;
}

////////////////////////////////////////////////////////////////////////////////

