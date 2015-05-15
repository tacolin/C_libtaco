#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "logger.h"
#include "queue.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...)\
{\
    if (assertion)\
    {\
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

struct logger
{
    struct queue mq;
    int flag;
    bool running;
    int level;

    struct udp udp;
    struct udp_addr remote;

    int fd;
};

static void _clean_msg(void* input)
{
    if (input) free(input);
}

struct logger* logger_create(int flag, char* dstip, int dstport, char* filepath)
{
    struct logger* lg = calloc(sizeof(struct logger), 1);

    lg->flag  = flag;
    lg->level = LOG_LV_DEFAULT;
    queue_init(&lg->mq, -1, _clean_msg, QUEUE_FLAG_POP_BLOCK);

    if (flag & LOG_FLAG_UDP)
    {
        CHECK_IF(dstip == NULL, goto _ERROR, "dstip is null");
        CHECK_IF(dstport <= 0, goto _ERROR, "dstport = %d invalid", dstport);

        udp_init(&lg->udp, NULL, UDP_PORT_ANY);
        snprintf(lg->remote.ipv4, INET_ADDRSTRLEN, "%s", dstip);
        lg->remote.port = dstport;
    }

    if (flag & LOG_FLAG_FILE)
    {
        CHECK_IF(filepath == NULL, goto _ERROR, "filepath is null");
        lg->fd = open(filepath, O_RDWR | O_CREAT, 0755);
    }
    return lg;

_ERROR:
    if (lg) logger_release(lg);
    return NULL;
}

void logger_release(struct logger* lg)
{
    CHECK_IF(lg == NULL, return, "lg is null");

    queue_clean(&lg->mq);

    if (lg->running) logger_break(lg);

    if (lg->flag & LOG_FLAG_UDP) udp_uninit(&lg->udp);

    if (lg->flag & LOG_FLAG_FILE) close(lg->fd);

    free(lg);
}

static void _handlemsg(struct logger* lg, char* msg)
{
    if (lg->flag & LOG_FLAG_STDOUT) fprintf(stdout, "%s", msg);

    if (lg->flag & LOG_FLAG_UDP) udp_send(&lg->udp, lg->remote, msg, strlen(msg)+1);

    if (lg->flag & LOG_FLAG_FILE) write(lg->fd, msg, strlen(msg)+1);
}

// void logger_handlemsg(struct logger* lg, char* msg)
// {
//     CHECK_IF(lg == NULL, return, "lg is null");
//     CHECK_IF(msg == NULL, return, "msg is null");
//     _handlemsg(lg, msg);
// }

void logger_run(struct logger* lg)
{
    CHECK_IF(lg == NULL, return, "lg is null");

    lg->running = true;
    while (lg->running)
    {
        char* msg = queue_pop(&lg->mq);
        CHECK_IF(msg == NULL, break, "msg is null");

        _handlemsg(lg, msg);
        free(msg);
    }
    lg->running = false;
    return;
}

void logger_break(struct logger* lg)
{
    CHECK_IF(lg == NULL, return, "lg is null");

    lg->running = false;
    log_print(lg, LOG_LV_INVALID, "stop\n");
    return;
}

void logger_setlevel(struct logger* lg, int level)
{
    CHECK_IF(lg == NULL, return, "lg is null");
    lg->level = level;
}

int logger_getlevel(struct logger* lg)
{
    CHECK_IF(lg == NULL, return LOG_LV_INVALID, "lg is null");
    return lg->level;
}

void logger_enq(struct logger* lg, char* str)
{
    CHECK_IF(lg == NULL, return, "lg is null");
    queue_push(&lg->mq, str);
}
