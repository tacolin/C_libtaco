#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

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

#define LOCK(h) atom_spinlock(&h->lock);
#define UNLOCK(h) atom_spinunlock(&h->lock);

struct history
{
    int lock;
    int data_size;
    int max_num;
    int head;
    int tail;
    void** data_array;
};

struct history* history_create(int data_size, int max_num)
{
    CHECK_IF(data_size <= 0, return NULL, "data_size = %d invalid", data_size);
    CHECK_IF(max_num <= 1, return NULL, "max_num = %d invalid", max_num);

    struct history* h = calloc(sizeof(struct history), 1);
    h->head       = 0;
    h->tail       = 0;
    h->lock       = 0;
    h->data_size  = data_size;
    h->max_num    = max_num;
    h->data_array = calloc(sizeof(void*), max_num);
    int i;
    for (i=0; i<max_num; i++)
    {
        h->data_array[i] = calloc(data_size, 1);
    }
    return h;
}

void history_release(struct history* h)
{
    CHECK_IF(h == NULL, return, "h is null");

    LOCK(h);
    int i;
    for (i=0; i<h->max_num; i++)
    {
        free(h->data_array[i]);
    }
    free(h->data_array);
    UNLOCK(h);
    free(h);
    return;
}

int history_add(struct history* h, void* data, int data_size)
{
    CHECK_IF(h == NULL, return HIS_FAIL, "h is null");
    CHECK_IF(data == NULL, return HIS_FAIL, "data is null");
    CHECK_IF(data_size > h->data_size, return HIS_FAIL, "data_size = %d > created data_size = %d", data_size, h->data_size);

    LOCK(h);
    {
        int tail = h->tail;
        tail++;
        if (tail >= h->max_num)
        {
            tail = 0;
        }
        memcpy(h->data_array[tail], data, data_size);

        if (tail == h->head)
        {
            h->head++;
            if (h->head >= h->max_num)
            {
                h->head = 0;
            }
        }
        h->tail = tail;
    }
    UNLOCK(h);
    return HIS_OK;
}

int history_addstr(struct history* h, char* str)
{
    CHECK_IF(h == NULL, return HIS_FAIL, "h is null");
    CHECK_IF(str == NULL, return HIS_FAIL, "str is null");

    int ret;
    if (strlen(str)+1 > h->data_size)
    {
        char tmp[h->data_size];
        snprintf(tmp, h->data_size, "%s", str);
        ret = history_add(h, tmp, h->data_size);
    }
    else
    {
        ret = history_add(h, str, strlen(str)+1);
    }
    return ret;
}

int history_do(struct history* h, int start, int end, void (*execfn)(int idx, void* data))
{
    CHECK_IF(h == NULL, return HIS_FAIL, "h is null");
    CHECK_IF(execfn == NULL, return HIS_FAIL, "execfn is null");
    CHECK_IF(start <= 0, return HIS_FAIL, "start = %d invalid", start);
    CHECK_IF(end > h->max_num + 1, return HIS_FAIL, "end = %d invalid", end);
    CHECK_IF(start > end, return HIS_FAIL, "start = %d and end = %d invalid", start, end);

    LOCK(h);
    {
        int head = (h->head + (start-1)) % h->max_num;
        int tail = (h->tail + end) % h->max_num;

        for (; ; start++)
        {
            execfn(start, h->data_array[head]);
            if (head == tail) break;

            head++;
            if (head >= h->max_num) head = 0;
        }
    }
    UNLOCK(h);
    return HIS_OK;
}

int history_do_all(struct history* h, void (*execfn)(int idx, void* data))
{
    return history_do(h, 1, h->max_num, execfn);
}

int history_clear(struct history* h)
{
    CHECK_IF(h == NULL, return HIS_FAIL, "h is null");
    LOCK(h);
    {
        h->head = 0;
        h->tail = 0;
    }
    UNLOCK(h);
    return HIS_OK;
}