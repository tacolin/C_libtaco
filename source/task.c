
#include "task.h"
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

////////////////////////////////////////////////////////////////////////////////

static tList _task_list;
static pthread_mutex_t _task_lock;
static int   _system_init_flag = 0;

////////////////////////////////////////////////////////////////////////////////

static void _msleep(int ms)
{
    struct timeval timeout;

    timeout.tv_sec = ms / 1000;
    timeout.tv_usec = (ms % 1000) * 1000;

    while (select(0, 0, 0, 0, &timeout))
    {
        // do nothing
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////

static void* _taskMainFunc(void* input)
{
    tTask* task = (tTask*)input;
    check_if(task == NULL, return, "task is null");

    task->is_stopped = 0;

    sem_post(&(task->stop_sem));
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    if (task->type == TASK_ONESHOT)
    {
        task->routine_fn(task, task->arg);
        task->is_stopped = 1;
        pthread_exit(NULL);
    }
    else
    {
        while (1)
        {
            if (task->stop_flag == 1)
            {
                pthread_testcancel();
                task->is_stopped = 1;
                pthread_exit(NULL);
                break;
            }

            task->routine_fn(task, task->arg);
            pthread_testcancel();

            if (task->type == TASK_PREEMPTIVE) _msleep(1); // sleep 1 ms
        }
    }

    return NULL;
}

static void _taskCleanFunc(void* arg)
{
    tTask* task = (tTask*)arg;

    check_if(task == NULL, return, "task is null");

    task->stop_flag = 1;
    pthread_cancel(task->thread);
    pthread_join(task->thread, NULL);

    dprint("please stop task (%s) before uninit task system", task->name);

    return;
}

////////////////////////////////////////////////////////////////////////////////

int task_init(tTask* task, char* name, tTaskRoutineFn routine_fn,
                      void* arg, tTaskPriority priority, tTaskType type)
{
    check_if(_system_init_flag == 0, return TASK_FAIL,"system is not initialized");
    check_if(task == NULL, return TASK_FAIL, "task is null");
    check_if(routine_fn == NULL, return TASK_FAIL ,"task routine function is null!");

    memset(task, 0, sizeof(tTask));

    if (name)
    {
        snprintf(task->name, TASK_NAME_SIZE, "%s", name);
    }

    if (priority < TASK_LOW)
    {
        priority = TASK_LOW;
    }
    else if (priority > TASK_HIGH)
    {
        priority = TASK_HIGH;
    }

    task->routine_fn = routine_fn;
    task->arg        = arg;
    task->type       = type;
    task->stop_flag  = 0;
    task->priority   = priority;

    sem_init(&(task->stop_sem), 0, 0);

    return TASK_OK;
}

int task_start(tTask* task)
{
    check_if(_system_init_flag == 0, return TASK_FAIL,"system is not initialized");
    check_if(task == NULL, return TASK_FAIL, "task is null");

    pthread_mutex_lock(&_task_lock);
    int list_ret = list_append(&_task_list, task);
    pthread_mutex_unlock(&_task_lock);

    check_if(list_ret != LIST_OK, return TASK_FAIL, "append task(%s) to list failed", task->name);

    pthread_attr_t *attr = &(task->attr);
    pthread_attr_init(attr);
    pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE);

    int ret = pthread_attr_setschedpolicy(attr, SCHED_FIFO);
    check_if(ret != 0, return TASK_FAIL, "task(%s) set sched policy failed", task->name);

    task->setting.sched_priority = task->priority;
    ret = pthread_attr_setschedparam(attr, &(task->setting));
    check_if(ret != 0, return TASK_FAIL, "task(%s) set sched priority failed", task->name);

    task->stop_flag = 0;

    ret = pthread_create(&(task->thread), attr, _taskMainFunc, (void*)task);
    check_if(ret != 0, return TASK_FAIL, "task(%s) start fail to pthread_create()!", task->name);

    sem_wait(&(task->stop_sem));

    return TASK_OK;
}

void task_stop(tTask* task)
{
    check_if(_system_init_flag == 0, return, "system is not initialized");
    check_if(task == NULL, return, "task is null");

    pthread_mutex_lock(&_task_lock);
    int list_ret = list_remove(&_task_list, task);
    pthread_mutex_unlock(&_task_lock);
    check_if(list_ret != LIST_OK, return, "remove task (%s) from running list failed", task->name);

    task->stop_flag = 1;

    // for safety
    while (task->is_stopped != 1)
    {
        _msleep(1);
    }

    pthread_cancel(task->thread);
    pthread_join(task->thread, NULL);

    return;
}

int task_system_init(void)
{
    check_if(_system_init_flag != 0, return TASK_FAIL, "system has been already initialized");

    int check = pthread_mutex_init(&_task_lock, NULL);
    check_if(check != 0, return TASK_FAIL, "pthread_mutext_init failed");

    list_init(&_task_list, _taskCleanFunc);

    _system_init_flag = 1;

    return TASK_OK;
}

int task_system_uninit(void)
{
    check_if(_system_init_flag == 0, return TASK_FAIL, "system is not initialized yet");

    int num = list_length(&_task_list);

    if ( num > 0 ) derror("there are %d running tasks!", num);

    list_clean(&_task_list);

    _system_init_flag = 0;

    return TASK_OK;
}
