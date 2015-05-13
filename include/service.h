#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <stdbool.h>
#include "map.h"
#include "fast_queue.h"

#define SERV_OK (0)
#define SERV_FAIL (-1)

#define SERVICE_NAME_SIZE (20)

#define MAX_SERVICE_NUM (100)
#define MAX_WATCH_NUM (1000)

#define INVALID_ID MAPID_INVALID

typedef unsigned int serviceid;
typedef unsigned int watchid;

typedef enum
{
    TAG_COPY = 0,
    TAG_DONTCOPY = 1,

} tag_t;

typedef enum
{
    RET_OK = 0,
    RET_DONTFREE = 1

} ret_t;

struct service_msg
{
    int session;
    serviceid src;
    void* msg;
    int msglen;
};

typedef ret_t (*service_cb)(serviceid self, void* db, int session, serviceid src, void* msg, int msglen);

struct service
{
    char name[SERVICE_NAME_SIZE+1];
    serviceid id;

    struct fqueue* mq;
    void* db;

    service_cb handlemsg;
    void (*init)(serviceid sid, void* db);
    void (*uninit)(serviceid sid, void* db);

    int epfd;

    struct map watchers;

    int qfd;
    int stopfd;

    bool watching;
};

struct watcher
{
    watchid id;
    int fd;
    void (*callback)(serviceid sid, void* db, int fd, void* arg);
    void* arg;

    int time_ms;
    int interval_ms;
    void (*tmcallback)(serviceid sid, void* db, void* arg);
    serviceid sid;
};

serviceid service_create(char* name, void* db, service_cb handlemsg, void (*init)(serviceid sid, void* db), void (*uninit)(serviceid sid, void* db));

serviceid service_getid(char* name);

watchid service_watch(serviceid sid, int fd, void (*callback)(serviceid sid, void* db, int fd, void* arg), void* arg);
void service_unwatch(serviceid sid, watchid wid);

watchid service_start_timer(serviceid sid, int time_ms, int interval_ms, void (*callback)(serviceid sid, void* db, void* arg), void* arg);
void service_stop_timer(serviceid sid, watchid wid);

int service_send(serviceid src, serviceid dst, tag_t tag, int session, void* msg, int msglen);

int service_system_init(void);
void service_system_uninit(void);

void service_system_run(void);
void service_system_break(void);

#endif //_SERVICE_H_
