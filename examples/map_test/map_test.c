#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "basic.h"
#include "simple_thread.h"
#include "map.h"

#define MAX_ID_NUM 100

static unsigned int _pool[MAX_ID_NUM] = {0};

static void _grab(tMap* map, int thread_no)
{
    int i;
    for (i=0; i<MAX_ID_NUM; i++)
    {
        // random grab one
        int r = rand() % (i+1);
        unsigned int id = _pool[r];
        void* ptr = map_grab(map, id);
        dprint("thread %d : grab %d, id = %u, ptr = %p", thread_no, r, id, ptr);
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
    _grab((tMap*)arg, 1);
}

static void _grab2(void* arg)
{
    _grab((tMap*)arg, 2);
}

static void _create(void* arg)
{
    tMap* map = (tMap*)arg;
    int i;
    for (i=0; i<MAX_ID_NUM; i++)
    {
        // add one
        map_add(map, (void*)((intptr_t)i+1), &_pool[i]);
        dprint("create %d id = %u", i, _pool[i]);
        usleep(50);

        // random release one
        int r = rand() % (i+1);
        unsigned int id = _pool[r];
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
    tMap map;
    unsigned int tmp[MAX_ID_NUM];

    map_init(&map, NULL);

    tSthread t[3] = {
        { _create, &map },
        { _grab1, &map },
        { _grab2, &map }
    };

    sthread_join(t, 3);

    map_uninit(&map);

    return 0;
}
