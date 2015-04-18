#include "oldevents.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#define OEV_MAX_QUEUE_NUM (1000)

////////////////////////////////////////////////////////////////////////////////

static tOevStatus _addIoToList(tOevLoop* loop, struct tOevIo* io)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(io == NULL, return OEV_ERROR, "io is null");
    check_if(io->prev != NULL, return OEV_ERROR, "io is already added to list");
    check_if(io->next != NULL, return OEV_ERROR, "io is already added to list");
    check_if(io->is_init != 1, return OEV_ERROR, "io is not init");

    if (loop->iolist == NULL)
    {
        // list is empty
        loop->iolist = io;
        return OEV_OK;
    }

    loop->iolist->prev = io;
    io->next           = loop->iolist;
    loop->iolist       = io;

    return OEV_OK;
}

static tOevStatus _delIoFromList(tOevLoop* loop, struct tOevIo* io)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(io == NULL, return OEV_ERROR, "io is null");
    check_if(io->is_init != 1, return OEV_ERROR, "io is not init");

    if (io->prev == NULL && io->next == NULL)
    {
        if (loop->iolist != io)
        {
            derror("io is not in list");
            return OEV_ERROR;
        }

        // io is head and tail
        loop->iolist = NULL;
    }
    else if (io->prev == NULL) // io is head
    {
        loop->iolist = io->next;
        io->next->prev = NULL;

        io->next = NULL;
    }
    else if (io->next == NULL) // io is tail
    {
        io->prev->next = NULL;

        io->prev = NULL;
    }
    else
    {
        io->prev->next = io->next;
        io->next->prev = io->prev;

        io->next = NULL;
        io->prev = NULL;
    }

    return OEV_OK;
}

static tOevStatus _cleanIoList(tOevLoop* loop)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");

    if (loop->iolist == NULL)
    {
        return OEV_OK;
    }

    tOevStatus ret;
    struct tOevIo* io;
    while (io = loop->iolist)
    {
        ret = _delIoFromList(loop, io);
        check_if(ret != OEV_OK, return ret, "_delIoFromList failed");
        io->tid = 0;
    }

    return OEV_OK;
}

////////////////////////////////////////////////////////////////////////////////

static tOevStatus _addTimerToList(tOevLoop* loop, tOevTimer* timer)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(timer == NULL, return OEV_ERROR, "timer is null");
    check_if(timer->prev != NULL, return OEV_ERROR, "timer is already added to list");
    check_if(timer->next != NULL, return OEV_ERROR, "timer is already added to list");
    check_if(timer->is_init != 1, return OEV_ERROR, "timer is not init");

    if (loop->timerlist == NULL)
    {
        // list is empty
        loop->timerlist = timer;
        return OEV_OK;
    }

    tOevTimer* target;
    for (target = loop->timerlist; target; target = target->next)
    {
        if (timer->rest_ms < target->rest_ms) // insert timer before target
        {
            if (target->prev == NULL) // target is head
            {
                target->prev    = timer;
                timer->prev     = NULL;
                timer->next     = target;
                loop->timerlist = timer;
            }
            else
            {
                target->prev->next = timer;
                timer->prev        = target->prev;

                target->prev = timer;
                timer->next  = target;
            }

            return OEV_OK;
        }
        else if (target->next == NULL) // append timer to list tail
        {
            target->next = timer;
            timer->prev  = target;
            timer->next  = NULL;

            return OEV_OK;
        }
    }

    derror("find no target...?");
    return OEV_ERROR;
}

static tOevStatus _delTimerFromList(tOevLoop* loop, tOevTimer* timer)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(loop->timerlist == NULL, return OEV_ERROR, "timer list is empty");

    check_if(timer == NULL, return OEV_ERROR, "timer is null");
    check_if(timer->is_init != 1, return OEV_ERROR, "timer is not init");

    if (timer->prev == NULL && timer->next == NULL)
    {
        if (loop->timerlist != timer)
        {
            derror("timer is not in list");
            return OEV_ERROR;
        }

        // timer is head and tail
        loop->timerlist = NULL;
    }
    else if (timer->prev == NULL) // timer is head
    {
        loop->timerlist   = timer->next;
        timer->next->prev = NULL;

        timer->next       = NULL;
    }
    else if (timer->next == NULL) // timer is tail
    {
        timer->prev->next = NULL;

        timer->prev = NULL;
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;

        timer->next = NULL;
        timer->prev = NULL;
    }

    return OEV_OK;
}

static tOevStatus _cleanTimerList(tOevLoop* loop)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");

    if (loop->timerlist == NULL)
    {
        return OEV_OK;
    }

    tOevTimer* timer;
    while (timer = loop->timerlist)
    {
        _delTimerFromList(loop, timer);
        timer->tid     = 0;
        timer->rest_ms = 0;
    }

    return OEV_OK;
}

////////////////////////////////////////////////////////////////////////////////

static tOevStatus _initQueue(tOevQueue* queue)
{
    check_if(queue == NULL, return OEV_ERROR, "queue is null");

    queue->head = NULL;
    queue->tail = NULL;

    queue->max_obj_num = OEV_MAX_QUEUE_NUM;
    queue->curr_obj_num = 0;

    int chk = pthread_mutex_init(&(queue->lock), NULL);
    check_if(chk != 0, return OEV_ERROR, "pthread_mutext_init failed");

    queue->is_init = 1;

    return OEV_OK;
}

static tOevStatus _cleanQueue(tOevQueue* queue)
{
    check_if(queue == NULL, return OEV_ERROR, "queue is null");
    check_if(queue->is_init != 1, return OEV_ERROR, "queue is not init yet");

    pthread_mutex_lock(&queue->lock);

    tOevQueueObj* obj = queue->head;
    tOevQueueObj* next;
    while (obj)
    {
        next = obj->next;

        if (obj->type == OEV_ONCE)
        {
            if (obj->ev) free(obj->ev);
        }

        free(obj);

        obj = next;
    }

    queue->curr_obj_num = 0;
    queue->head         = NULL;
    queue->tail         = NULL;

    pthread_mutex_unlock(&queue->lock);

    return OEV_OK;
}

static tOevStatus _enQueue(tOevQueue* queue, tOevQueueObj* obj)
{
    check_if(queue == NULL, return OEV_ERROR, "queue is null");
    check_if(obj == NULL, return OEV_ERROR, "obj is null");
    check_if(queue->is_init != 1, return OEV_ERROR, "queue is not init yet");

    if (queue->curr_obj_num >= queue->max_obj_num)
    {
        derror("queue is full, curr_obj_num = %d", queue->curr_obj_num);
        return OEV_ERROR;
    }

    pthread_mutex_lock(&queue->lock);

    if (queue->tail)
    {
        queue->tail->next = obj;
        queue->tail = obj;
    }
    else
    {
        queue->tail = obj;
        queue->head = obj;
    }

    queue->curr_obj_num++;

    pthread_mutex_unlock(&queue->lock);

    return OEV_OK;
}

static tOevQueueObj* _deQueue(tOevQueue* queue)
{
    check_if(queue == NULL, return NULL, "queue is null");
    check_if(queue->is_init != 1, return NULL, "queue is not init yet");

    if (queue->curr_obj_num <= 0)
    {
        return NULL;
    }

    pthread_mutex_lock(&queue->lock);

    tOevQueueObj* obj = queue->head;
    if (obj->next)
    {
        queue->head = obj->next;
    }
    else
    {
        queue->head = NULL;
        queue->tail = NULL;
    }

    obj->next = NULL;

    queue->curr_obj_num--;

    pthread_mutex_unlock(&queue->lock);

    return obj;
}

static tOevQueueObj* _newQueueObj(tOevType type, tOevAction action, void* ev)
{
    check_if(ev == NULL, return NULL, "ev is null");

    tOevQueueObj* obj = calloc(sizeof(tOevQueueObj), 1);

    obj->type   = type;
    obj->action = action;
    obj->ev     = ev;
    obj->next   = NULL;

    return obj;
}

////////////////////////////////////////////////////////////////////////////////

tOevStatus oevloop_init(tOevLoop* loop)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");

    memset(loop, 0, sizeof(tOevLoop));

    loop->timerlist = NULL;
    loop->iolist    = NULL;

    tOevStatus ret = _initQueue(&loop->once_queue);
    check_if(ret != OEV_OK, return ret, "_initQueue once_queue failed");

    ret = _initQueue(&loop->inter_thread_queue);
    check_if(ret != OEV_OK, return ret, "_initQueue inter_thread_queue failed");

    loop->is_running = 0;
    loop->tid        = 0;
    loop->is_init    = 1;

    return OEV_OK;
}

tOevStatus oevloop_uninit(tOevLoop* loop)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");

    if (loop->is_running)
    {
        oevloop_break(loop);
    }

    if (loop->iolist)
    {
        _cleanIoList(loop);
        loop->iolist = NULL;
    }

    if (loop->timerlist)
    {
        _cleanTimerList(loop);
        loop->timerlist = NULL;
    }

    loop->is_init = 0;

    _cleanQueue(&loop->once_queue);
    _cleanQueue(&loop->inter_thread_queue);

    loop->tid = 0;

    return OEV_OK;
}

static tOevStatus _handleTimerEvent(tOevLoop* loop, struct timeval timeout)
{
    long tick_rest_ms = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
    long consume_ms   = OEV_TICK_MS - tick_rest_ms;

    tOevTimer* timer = loop->timerlist;
    tOevTimer* next_timer;

    while (timer && loop->is_running)
    {
        next_timer = timer->next;

        timer->rest_ms -= consume_ms;

        if (timer->rest_ms <= 0)
        {
            timer->callback(loop, timer, timer->arg);

            if (timer->type == OEV_TIMER_PERIODIC)
            {
                timer->rest_ms = timer->period_ms;
            }
            else
            {
                timer->rest_ms = 0;
                _delTimerFromList(loop, timer);
            }
        }

        timer = next_timer;
    }

    return OEV_OK;
}

static tOevStatus _handleOnceEvent(tOevLoop* loop)
{
    tOevQueueObj* obj;
    tOevOnce* once;

    while ((obj = _deQueue(&loop->once_queue)) && loop->is_running)
    {
        if (obj->ev == NULL)
        {
            derror("obj->ev is null");
            free(obj);
            return OEV_ERROR;
        }

        if (obj->type != OEV_ONCE)
        {
            derror("type = %d but in once_queue", obj->type);
            free(obj);
            return OEV_ERROR;
        }

        once = (tOevOnce*)obj->ev;
        once->callback(loop, NULL, once->arg);

        free(once);
        free(obj);
    }

    return OEV_OK;
}

static tOevStatus _handleInterThreadEvent(tOevLoop* loop)
{
    tOevQueueObj* obj;
    tOevOnce* once;

    while ((obj = _deQueue(&loop->inter_thread_queue)) && loop->is_running)
    {
        if (obj->ev == NULL)
        {
            derror("obj->ev is null");
            free(obj);
            return OEV_ERROR;
        }

        if (obj->type == OEV_IO)
        {
            if (obj->action == OEV_IO_START)
            {
                oevio_start(loop, (struct tOevIo*)obj->ev);
            }
            else if (obj->action == OEV_IO_STOP)
            {
                oevio_stop(loop, (struct tOevIo*)obj->ev);
            }
            else
            {
                derror("unknown oev action for io = %d", obj->action);
                free(obj);
                return OEV_ERROR;
            }
        }
        else if (obj->type == OEV_TIMER)
        {
            if (obj->action == OEV_TM_START)
            {
                oevtm_start(loop, (tOevTimer*)obj->ev);
            }
            else if (obj->action == OEV_TM_STOP)
            {
                oevtm_stop(loop, (tOevTimer*)obj->ev);
            }
            else if (obj->action == OEV_TM_RESTART)
            {
                oevtm_restart(loop, (tOevTimer*)obj->ev);
            }
            else if (obj->action == OEV_TM_PAUSE)
            {
                oevtm_pause(loop, (tOevTimer*)obj->ev);
            }
            else if (obj->action == OEV_TM_RESUME)
            {
                oevtm_resume(loop, (tOevTimer*)obj->ev);
            }
            else
            {
                derror("unknown oev action for timer = %d", obj->action);
                free(obj);
                return OEV_ERROR;
            }
        }
        else if (obj->type == OEV_ONCE)
        {
            once = (tOevOnce*)obj->ev;
            oev_once(loop, once->callback, once->arg);
            free(once);
        }
        else
        {
            derror("unknown oev type = %d", obj->type);
            free(obj);
            return OEV_ERROR;
        }

        free(obj);
    }

    return OEV_OK;
}

tOevStatus oevloop_run(tOevLoop* loop)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");

    int            select_ret = -1;
    fd_set         readset;
    struct timeval timeout    = {};
    tOevStatus     ret        = OEV_ERROR;
    struct tOevIo*        io         = NULL;

    loop->is_running       = 1;
    loop->is_still_running = 1;
    loop->tid              = pthread_self();

    ////////////////////////////////////////////////////////////////////////////
    // handle events which are started (or added) before loop running
    ////////////////////////////////////////////////////////////////////////////
    ret = _handleInterThreadEvent(loop);
    check_if(ret != OEV_OK, return OEV_ERROR, "handle events started before loop running failed");

    while (loop->is_running)
    {
        timeout.tv_sec  = OEV_TICK_MS / 1000;
        timeout.tv_usec = (OEV_TICK_MS % 1000) * 1000;

        FD_ZERO(&readset);

        for (io = loop->iolist; io; io = io->next)
        {
            FD_SET(io->fd, &readset);
        }

        select_ret = select(FD_SETSIZE, &readset, NULL ,NULL, &timeout);
        if (select_ret < 0)
        {
            loop->is_running = 0;
            derror("select failed");
            return OEV_ERROR;
        }
        else if (select_ret > 0)
        {
            ////////////////////////////////////////////////////////////////////
            // handle IO event
            ////////////////////////////////////////////////////////////////////
            for (io = loop->iolist; io && loop->is_running; io = io->next)
            {
                if (FD_ISSET(io->fd, &readset))
                {
                    io->callback(loop, io, io->arg);
                }
            }
        }

        ret = _handleTimerEvent(loop, timeout);
        check_if(ret != OEV_OK, return OEV_ERROR, "_handleTimerEvent failed");

        ret = _handleOnceEvent(loop);
        check_if(ret != OEV_OK, return OEV_ERROR, "_handleOnceEvent failed");

        ret = _handleInterThreadEvent(loop);
        check_if(ret != OEV_OK, return OEV_ERROR, "_handleInterThreadEvent failed");

    }

    loop->is_still_running = 0;
    loop->tid              = 0;

    return OEV_OK;
}

tOevStatus oevloop_break(tOevLoop* loop)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(loop->is_running != 1, return OEV_ERROR, "loop is not running");

    loop->is_running = 0;

    if (pthread_self() != loop->tid) // inter_thread break
    {
        struct timeval timeout;

        while (loop->is_still_running)
        {
            timeout.tv_sec  = OEV_TICK_MS / 1000;
            timeout.tv_usec = (OEV_TICK_MS % 1000) * 1000;

            select(0, NULL, NULL, NULL, &timeout);
        }
    }

    return OEV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tOevStatus oevio_init(struct tOevIo* io, tOevCallback callback, int fd, void* arg)
{
    check_if(io == NULL, return OEV_ERROR, "io is null");
    check_if(fd < 0, return OEV_ERROR, "fd = %d invalid", fd);
    check_if(callback == NULL, return OEV_ERROR, "callback is null");

    memset(io, 0, sizeof(struct tOevIo));

    io->fd       = fd;
    io->callback = callback;
    io->arg      = arg;
    io->tid      = 0;
    io->prev     = NULL;
    io->next     = NULL;
    io->is_init  = 1;

    return OEV_OK;
}

tOevStatus oevio_start(tOevLoop* loop, struct tOevIo* io)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(io == NULL, return OEV_ERROR, "io is null");
    check_if(io->is_init != 1, return OEV_ERROR, "io is not init yet");
    check_if(io->tid != 0, return OEV_ERROR, "io is already started");

    tOevStatus ret;

    if (pthread_self() != loop->tid) // inter_thread
    {
        tOevQueueObj* obj = _newQueueObj(OEV_IO, OEV_IO_START, io);
        check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

        ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue failed");

        return OEV_OK;
    }

    ret = _addIoToList(loop, io);
    check_if(ret != OEV_OK, return ret, "_addIoToList failed");

    io->tid = loop->tid;

    return OEV_OK;
}

tOevStatus oevio_stop(tOevLoop* loop, struct tOevIo* io)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(io == NULL, return OEV_ERROR, "io is null");
    check_if(io->is_init != 1, return OEV_ERROR, "io is not init yet");
    check_if(io->tid == 0, return OEV_ERROR, "io is not started");

    tOevStatus ret;

    if (pthread_self() != loop->tid) // inter_thread
    {
        tOevQueueObj* obj = _newQueueObj(OEV_IO, OEV_IO_STOP, io);
        check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

        ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue failed");

        return OEV_OK;
    }

    ret = _delIoFromList(loop, io);
    check_if(ret != OEV_OK, return ret, "_delIoFromList failed");

    io->tid = 0;

    return OEV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tOevStatus oevtm_init(tOevTimer* tm, tOevCallback callback, int period_ms, void* arg, tOevTimerType type)
{
    check_if(tm == NULL, return OEV_ERROR, "tm is null");
    check_if(callback == NULL, return OEV_ERROR, "callback is null");
    check_if(period_ms < 0, return OEV_ERROR, "period_ms = %d invalid", period_ms);

    memset(tm, 0, sizeof(tOevTimer));

    tm->type      = type;
    tm->rest_ms   = 0;
    tm->period_ms = period_ms;
    tm->callback  = callback;
    tm->arg       = arg;
    tm->tid       = 0;
    tm->prev      = NULL;
    tm->next      = NULL;
    tm->is_init   = 1;

    return OEV_OK;
}

tOevStatus oevtm_start(tOevLoop* loop, tOevTimer* tm)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return OEV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return OEV_ERROR, "tm is not init yet");
    check_if(tm->tid != 0, return OEV_ERROR, "tm is already started");

    tOevStatus ret;

    if (pthread_self() != loop->tid)
    {
        tOevQueueObj* obj = _newQueueObj(OEV_TIMER, OEV_TM_START, tm);
        check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

        ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue failed");

        return OEV_OK;
    }

    tm->rest_ms = tm->period_ms;

    ret = _addTimerToList(loop, tm);
    check_if(ret != OEV_OK, return ret, "_addTimerToList failed");

    tm->tid = loop->tid;

    return OEV_OK;
}

tOevStatus oevtm_stop(tOevLoop* loop, tOevTimer* tm)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return OEV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return OEV_ERROR, "tm is not init yet");
    check_if(tm->tid == 0, return OEV_ERROR, "tm is not started");

    tOevStatus ret;

    if (pthread_self() != loop->tid)
    {
        tOevQueueObj* obj = _newQueueObj(OEV_TIMER, OEV_TM_STOP, tm);
        check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

        ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue failed");

        return OEV_OK;
    }

    _delTimerFromList(loop, tm);

    tm->rest_ms = 0;

    tm->tid = 0;

    return OEV_OK;
}

tOevStatus oevtm_restart(tOevLoop* loop, tOevTimer* tm)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return OEV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return OEV_ERROR, "tm is not init yet");

    tOevStatus ret;

    if (pthread_self() != loop->tid)
    {
        tOevQueueObj* obj = _newQueueObj(OEV_TIMER, OEV_TM_RESTART, tm);
        check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

        ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue failed");

        return OEV_OK;
    }

    if (tm->tid != 0) // timer is started
    {
        ret = oevtm_stop(loop, tm);
        check_if(ret != OEV_OK, return ret, "oevtm_stop failed");
    }

    ret = oevtm_start(loop, tm);
    check_if(ret != OEV_OK, return ret, "oevtm_start failed");

    return OEV_OK;
}

tOevStatus oevtm_pause(tOevLoop* loop, tOevTimer* tm)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return OEV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return OEV_ERROR, "tm is not init yet");
    check_if(tm->tid == 0, return OEV_ERROR, "tm is not started");

    if (pthread_self() != loop->tid)
    {
        tOevQueueObj* obj = _newQueueObj(OEV_TIMER, OEV_TM_PAUSE, tm);
        check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

        tOevStatus ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue failed");

        return OEV_OK;
    }

    _delTimerFromList(loop, tm);

    tm->tid = 0;

    return OEV_OK;
}

tOevStatus oevtm_resume(tOevLoop* loop, tOevTimer* tm)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(tm == NULL, return OEV_ERROR, "tm is null");
    check_if(tm->is_init != 1, return OEV_ERROR, "tm is not init yet");
    check_if(tm->tid != 0, return OEV_ERROR, "tm is already started");

    tOevStatus ret;

    if (pthread_self() != loop->tid)
    {
        tOevQueueObj* obj = _newQueueObj(OEV_TIMER, OEV_TM_RESUME, tm);
        check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

        ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue failed");

        return OEV_OK;
    }

    ret = _addTimerToList(loop, tm);
    check_if(ret != OEV_OK, return ret, "_addTimerToList failed");

    tm->tid = loop->tid;

    return OEV_OK;
}

////////////////////////////////////////////////////////////////////////////////

tOevStatus oev_once(tOevLoop* loop, tOevCallback callback, void* arg)
{
    check_if(loop == NULL, return OEV_ERROR, "loop is null");
    check_if(loop->is_init != 1, return OEV_ERROR, "loop is not init yet");
    check_if(callback == NULL, return OEV_ERROR, "callback is null");

    tOevOnce* once = calloc(sizeof(tOevOnce), 1);
    once->callback = callback;
    once->arg      = arg;
    once->tid      = 0;
    once->is_init  = 1;

    tOevQueueObj* obj = _newQueueObj(OEV_ONCE, OEV_ONCE_ONCE, once);
    check_if(obj == NULL, return OEV_ERROR, "_newQueueObj failed");

    tOevStatus ret;

    if (pthread_self() != loop->tid)
    {
        ret = _enQueue(&loop->inter_thread_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue inter_thread_queue failed");
    }
    else
    {
        once->tid = loop->tid;
        ret       = _enQueue(&loop->once_queue, obj);
        check_if(ret != OEV_OK, return ret, "_enQueue once_queue failed");
    }

    return OEV_OK;
}
