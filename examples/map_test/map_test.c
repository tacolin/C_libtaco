#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "basic.h"
#include "task.h"
#include "map.h"

#define MAX_ID_NUM 1000

static int _running = 1;
static unsigned int _idPool[MAX_ID_NUM] = {0};

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    dprint("stop main()");
}

static void _grabRoutine(void* task, void* arg)
{
    check_if(arg == NULL, return, "arg is null");

    tMap* map = (tMap*)arg;
    tTask* t = (tTask*)task;

    int i;
    int r;
    unsigned int tmpid;
    void* ptr;
    tMapStatus ret;
    for (i=0; i<MAX_ID_NUM; i++)
    {
        r = rand() % (i+1);
        tmpid = _idPool[r];
        ptr = map_grab(map, tmpid);
        if (ptr)
        {
            dprint("task %s: grab %d ok, id = %u, ptr = %p", t->name, i, tmpid, ptr);
        }
        else
        {
            dprint("task %s: grab %d failed, id = %u", t->name, i, tmpid);
        }

        usleep(50);

        if (ptr) map_release(map, tmpid);
    }

    return;
}

static void _createRoutine(void* task, void* arg)
{
    check_if(arg == NULL, return, "arg is null");

    tMap* map = (tMap*)arg;
    int i;
    int r;
    unsigned int tmpid;
    void* ptr;
    tMapStatus ret;

    for (i=0; i<MAX_ID_NUM; i++)
    {
        ret = map_add(map, (void*)((intptr_t)i+1), &_idPool[i]);
        dprint("create %d id = %d", i, _idPool[i]);

        usleep(50);

        r = rand() % (i+1);
        tmpid = _idPool[r];

        ptr = map_release(map, tmpid);
        if (ptr)
        {
            dprint("release %d ok id = %u ptr = %p", i, tmpid, ptr);
        }
        else
        {
            dprint("release %d failed id = %u", i, tmpid);
        }
    }

    return;
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    task_system_init();

    tTask create = {};
    tTask grab1 = {};
    tTask grab2 = {};
    tMap map = {};

    map_init(&map, NULL);

    task_init(&create, "CREATE MAP", _createRoutine, &map, TASK_NORMAL, TASK_ONESHOT);
    task_init(&grab1, "GRAB 1 MAP", _grabRoutine, &map, TASK_NORMAL, TASK_ONESHOT);
    task_init(&grab2, "GRAB 2 MAP", _grabRoutine, &map, TASK_NORMAL, TASK_ONESHOT);

    task_start(&create);
    task_start(&grab1);
    task_start(&grab2);

    int i;
    for (i = 0; i < 10 & _running; ++i)
    {
        sleep(1);
    }

    task_stop(&create);
    task_stop(&grab1);
    task_stop(&grab2);

    map_uninit(&map);

    task_system_uninit();

    return 0;
}
