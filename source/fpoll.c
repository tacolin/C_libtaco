#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "fpoll.h"
#include "list.h"

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#define LOCK_DATA(rec) atom_spinlock(&rec->lock)
#define UNLOCK_DATA(rec) atom_spinunlock(&rec->lock)

#define LOCK_FPOLL() atom_spinlock(&_fpoll_lock);
#define UNLOCK_FPOLL() atom_spinunlock(&_fpoll_lock);

struct fpoll_rec
{
    int fpd;
    int lock;
    struct list data_list; // struct fpoll_data
};

static struct list _fpoll_list; // struct fpoll_rec
static int _fpoll_lock = 0;

////////////////////////////////////////////////////////////////////////////////

static void _clean_fpoll_data(void* input)
{
    CHECK_IF(input == NULL, return, "input is null");
    free(input);
}

static void _clean_fpoll_rec(void* input)
{
    CHECK_IF(input == NULL, return, "input is null");

    struct fpoll_rec* fpoll = (struct fpoll_rec*)input;
    close(fpoll->fpd);

    LOCK_DATA(fpoll);
    list_clean(&fpoll->data_list);
    UNLOCK_DATA(fpoll);
    free(input);
    return;
}

static int _find_fpoll(void* input, void* arg)
{
    CHECK_IF(input == NULL, return 0, "input is null");
    CHECK_IF(arg == NULL, return 0, "arg is null");

    struct fpoll_rec* fpoll = (struct fpoll_rec*)input;
    int fpd = *((int*)arg);
    return (fpoll->fpd == fpd) ? 1 : 0;
}

static int _find_fpoll_data(void* input, void* arg)
{
    CHECK_IF(input == NULL, return 0, "input is null");
    CHECK_IF(arg == NULL, return 0, "arg is null");

    struct fpoll_data* data = (struct fpoll_data*)input;
    int fd = *((int*)arg);
    return (data->fd == fd) ? 1 : 0;
}

int fpoll_system_init(void)
{
    _fpoll_lock = 0;
    list_init(&_fpoll_list, _clean_fpoll_rec);
    return 0;
}

void fpoll_system_uninit(void)
{
    LOCK_FPOLL();
    list_clean(&_fpoll_list);
    UNLOCK_FPOLL();
    return;
}

int fpoll_create(int max)
{
    int fpd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_IF(fpd < 0, return -1, "socket failed");

    struct fpoll_rec* fpoll = calloc(sizeof(struct fpoll_rec), 1);
    fpoll->fpd = fpd;
    list_init(&fpoll->data_list, _clean_fpoll_data);
    fpoll->lock = 0;

    LOCK_FPOLL();
    list_append(&_fpoll_list, fpoll);
    UNLOCK_FPOLL();
    return fpd;
}

void fpoll_close(int fpd)
{
    LOCK_FPOLL();
    void* data = list_find(&_fpoll_list, _find_fpoll, &fpd);
    if (data)
    {
        list_remove(&_fpoll_list, data);
        _clean_fpoll_rec(data);
    }
    UNLOCK_FPOLL();
    return;
}

int fpoll_wait(int fpd, struct fpoll_event *events, int maxevents, int timeout)
{
    CHECK_IF(fpd < 0, return -1, "fpd = %d invalid", fpd);
    CHECK_IF(events == NULL, return -1, "events is null");
    CHECK_IF(maxevents <= 0, return -1, "maxevents = %d invalid", maxevents);

    LOCK_FPOLL();
    struct fpoll_rec* fpoll = (struct fpoll_rec*)list_find(&_fpoll_list, _find_fpoll, &fpd);
    UNLOCK_FPOLL();
    CHECK_IF(fpoll == NULL, return -1, "find no fpoll with fpd = %d", fpd);

    fd_set readset;
    struct timeval period;
    void* obj;
    struct fpoll_data* data;
    int ret;

    FD_ZERO(&readset);

    LOCK_DATA(fpoll);
    LIST_FOREACH(&fpoll->data_list, obj, data)
    {
        FD_SET(data->fd, &readset);
    }
    UNLOCK_DATA(fpoll);

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

    CHECK_IF(ret < 0, return -1, "select failed");

    if (ret == 0)
    {
        return 0;
    }

    // ret > 0

    int count = 0;
    LOCK_DATA(fpoll);
    LIST_FOREACH(&fpoll->data_list, obj, data)
    {
        if (count > maxevents) break;

        if (FD_ISSET(data->fd, &readset))
        {
            events[count].data = *data;
            count++;
        }
    }
    UNLOCK_DATA(fpoll);

    return count;
}

int fpoll_ctl(int fpd, int op, int fd, struct fpoll_event *event)
{
    CHECK_IF(fpd < 0, return FPOLL_FAIL, "fpd = %d invalid", fpd);
    CHECK_IF(event == NULL, return FPOLL_FAIL, "event is null");
    CHECK_IF(fd < 0, return FPOLL_FAIL, "fd = %d invalid", fd);

    LOCK_FPOLL();
    struct fpoll_rec* fpoll = (struct fpoll_rec*)list_find(&_fpoll_list, _find_fpoll, &fpd);
    UNLOCK_FPOLL();
    CHECK_IF(fpoll == NULL, return FPOLL_FAIL, "find no fpoll with fpd = %d", fpd);

    if (op == FPOLL_CTL_ADD)
    {
        struct fpoll_data* data = calloc(sizeof(struct fpoll_data), 1);
        *data = event->data;

        data->fd = fd;

        LOCK_DATA(fpoll);
        list_append(&fpoll->data_list, data);
        UNLOCK_DATA(fpoll);
    }
    else if (op == FPOLL_CTL_DEL)
    {
        LOCK_DATA(fpoll);
        struct fpoll_data* data = (struct fpoll_data*)list_find(&fpoll->data_list, _find_fpoll_data, &fd);
        UNLOCK_DATA(fpoll);

        CHECK_IF(data == NULL, return FPOLL_FAIL, "find no struct fpoll_data with fd = %d", fd);

        LOCK_DATA(fpoll);
        list_remove(&fpoll->data_list, data);
        UNLOCK_DATA(fpoll);

        _clean_fpoll_data(data);
    }
    else
    {
        derror("unknown op = %d", op);
        return FPOLL_FAIL;
    }
    return FPOLL_OK;
}
