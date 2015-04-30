#ifndef _TASK_H_
#define _TASK_H_

#include "basic.h"
#include "list.h"
#include <pthread.h>
#include <semaphore.h>

////////////////////////////////////////////////////////////////////////////////

#define TASK_NAME_SIZE 20

#define TASK_OK (0)
#define TASK_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    TASK_LOW = 25,
    TASK_NORMAL = 50,
    TASK_HIGH = 75,

    TASK_PRIORITIES

} tTaskPriority;

typedef enum
{
    TASK_ONESHOT,
    TASK_PREEMPTIVE,
    TASK_NON_PREEMPTIVE,

    TASK_TYPES

} tTaskType;

typedef void (*tTaskRoutineFn)(void* task, void* arg);

typedef struct tTask
{
    char name[TASK_NAME_SIZE+1];

    void* arg;
    tTaskType type;
    tTaskRoutineFn routine_fn;
    tTaskPriority  priority;

    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param  setting;

    sem_t stop_sem;
    int stop_flag;
    int is_stopped;

} tTask;

////////////////////////////////////////////////////////////////////////////////

int task_init(tTask* task, char* name, tTaskRoutineFn routine_fn,
                      void* arg, tTaskPriority priority, tTaskType type);

int task_start(tTask* task);
void task_stop(tTask* task);

int task_system_init(void);
int task_system_uninit(void);

#endif //_TASK_H_
