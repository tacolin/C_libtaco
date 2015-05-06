#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "basic.h"
#include "thread.h"
#include "map.h"

#define MAX_ID_NUM 1000

static mapid _pool[MAX_ID_NUM] = {0};

static void _grab(struct map* map, int thread_no)
{
    int i;
    for (i=0; i<MAX_ID_NUM; i++)
    {
        // random grab one
        int r = rand() % (i+1);
        mapid id = _pool[r];
        void* ptr = map_grab(map, id);
        if (ptr)
        {
            dprint("thread %d : grab %d, id = %u, ptr = %p", thread_no, r, id, ptr);
        }
        else
        {
            dprint("thread %d : grab %d, id = %u, ptr = NULL", thread_no, r, id);
        }
        usleep(50);
        if (ptr)
        {
            ptr = map_release(map, id);
            if (ptr)
            {
                dprint("thread %d : release %d, id = %u, ptr = %p", thread_no, r, id, ptr);
            }
        }
    }
}

static void _grab1(void* arg)
{
    _grab((struct map*)arg, 1);
}

static void _grab2(void* arg)
{
    _grab((struct map*)arg, 2);
}

static void _create(void* arg)
{
    struct map* map = (struct map*)arg;
    int i;
    for (i=0; i<MAX_ID_NUM; i++)
    {
        // add one
        _pool[i] = map_new(map, (void*)((intptr_t)i+1));
        dprint("create %d id = %u", i, _pool[i]);
        usleep(50);

        // random release one
        int r = rand() % (i+1);
        mapid id = _pool[r];
        void* ptr = map_release(map, id);
        if (ptr)
        {
            dprint("release %d, id = %u, ptr = %p", r, id, ptr);
        }
        else
        {
            dprint("release %d failed, id = %u", r, id);
        }
    }
}

int main(int argc, char const *argv[])
{
    struct map map;
    mapid tmp[MAX_ID_NUM];

    map_init(&map, NULL);

    struct thread t[3] = {
        { _create, &map },
        { _grab1, &map },
        { _grab2, &map }
    };

    thread_join(t, 3);

    map_uninit(&map);

    return 0;
}
