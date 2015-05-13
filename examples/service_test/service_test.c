#include "basic.h"
#include "service.h"

struct taco
{
    watchid input;
    watchid timer;
    serviceid other;
};

static void _timeout(serviceid sid, void* db, void* arg)
{
    char buf[] = "fuckyou req";
    struct taco* t = (struct taco*)db;
    dprint("%s sent", buf);
    service_send(sid, t->other, 0, 0, buf, strlen(buf)+1);
}

static void _proc_stdin(serviceid sid, void* db, int fd, void* arg)
{
    char buf[256] = {0};
    int  readlen = read(fd, buf, 256);
    CHECK_IF(readlen <= 0, return, "read failed");

    buf[readlen-1] = (buf[readlen-1] == '\n') ? '\0' : buf[readlen-1];

    if (strcmp(buf, "exit") == 0)
    {
        service_system_break();
    }
    else
    {
        dprint("unkown : %s", buf);
    }
}

static void _init_taco1(serviceid sid, void* db)
{
    struct taco* taco = (struct taco*)db;
    taco->timer = service_start_timer(sid, 3000, 3000, _timeout, NULL);
    taco->input = service_watch(sid, 0, _proc_stdin, NULL);
    taco->other = service_getid("taco2");
}

static ret_t _handle_taco1(serviceid self, void* db, int session, serviceid src, void* msg, int msglen)
{
    dprint("%s recv", (char*)msg);
    return RET_OK;
}

static void _init_taco2(serviceid sid, void* db)
{
    struct taco* taco = (struct taco*)db;
    taco->other = service_getid("taco1");
}

static ret_t _handle_taco2(serviceid self, void* db, int session, serviceid src, void* msg, int msglen)
{
    dprint("%s recv", (char*)msg);

    char buf[] = "fuckyou rsp";
    dprint("%s send1", buf);
    service_send(self, src, 0, 0, buf, strlen(buf)+1);
    dprint("%s send2", buf);
    service_send(self, src, 0, 0, buf, strlen(buf)+1);
    dprint("%s send3", buf);
    service_send(self, src, 0, 0, buf, strlen(buf)+1);

    return RET_OK;
}

int main(int argc, char const *argv[])
{
    struct taco db1 = {};
    struct taco db2 = {};

    service_system_init();
    service_create("taco1", &db1, _handle_taco1, _init_taco1, NULL);
    service_create("taco2", &db2, _handle_taco2, _init_taco2, NULL);
    service_system_run();
    service_system_uninit();
    dprint("ok");
    return 0;
}
