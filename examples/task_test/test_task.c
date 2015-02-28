#include <signal.h>

#include "basic.h"
#include "task.h"

typedef struct tTaco
{
    char name[20];
    int count;

} tTaco;

static int _running = 1;

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    dprint("stop main()");
}

static void _routine(void* task, void* arg)
{
    tTaco* taco = (tTaco*)arg;

    dprint("%s count = %d", taco->name, taco->count);
    taco->count++;
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    task_system_init();

    tTask task1;
    tTask task2;
    tTask task3;

    tTaco data1 = {};
    tTaco data2 = {};
    tTaco data3 = {};

    snprintf(data1.name, 20, "data1");
    snprintf(data2.name, 20, "data2");
    snprintf(data3.name, 20, "data3");

    task_init(&task1, "TEST TASK1", _routine, &data1, TASK_LOW, TASK_ONESHOT);
    task_init(&task2, "TEST TASK2", _routine, &data2, TASK_LOW, TASK_PREEMPTIVE);
    task_init(&task3, "TEST TASK3", _routine, &data3, TASK_HIGH, TASK_NON_PREEMPTIVE);

    task_start(&task1);
    task_start(&task2);
    task_start(&task3);

    int i;
    for (i = 0; i < 10 & _running; ++i)
    {
        sleep(1);
    }

    task_stop(&task1);
    task_stop(&task2);
    task_stop(&task3);

    task_system_uninit();

    return 0;
}
