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

int logger_unset_udp(struct logger* logger)
{
    CHECK_IF(logger == NULL, return LOG_FAIL, "logger is null");
    logger->flag &= ~LOG_FLAG_UDP;
    udp_uninit(&logger->udp);
    return LOG_OK;
}

int logger_set_udp(struct logger* logger, char* dstip, int dstport)
{
    CHECK_IF(logger == NULL, return LOG_FAIL, "logger is null");
    CHECK_IF(dstip == NULL, return LOG_FAIL, "dstip is null");
    CHECK_IF(dstport <= 0, return LOG_FAIL, "dstport = %d invalid", dstport);

    if (logger->udp.is_init)
    {
        logger_unset_udp(logger);
    }

    udp_init(&logger->udp, NULL, UDP_PORT_ANY);
    snprintf(logger->remote.ipv4, INET_ADDRSTRLEN, "%s", dstip);
    logger->remote.port = dstport;
    logger->flag |= LOG_FLAG_UDP;
    return LOG_OK;
}

int logger_unset_file(struct logger* logger)
{
    CHECK_IF(logger == NULL, return LOG_FAIL, "logger is null");
    logger->flag &= ~LOG_FLAG_FILE;
    close(logger->fd);
    logger->fd = -1;
    return LOG_OK;
}

int logger_set_file(struct logger* logger, char* filepath)
{
    CHECK_IF(logger == NULL, return LOG_FAIL, "logger is null");
    CHECK_IF(filepath == NULL, return LOG_FAIL, "filepath is null");

    if (logger->fd > 0)
    {
        logger_unset_file(logger);
    }

    unlink(filepath);

    logger->fd = open(filepath, O_RDWR | O_CREAT, 0755);
    CHECK_IF(logger->fd <= 0, return LOG_FAIL, "open failed");

    logger->flag |= LOG_FLAG_FILE;
    return LOG_OK;
}

int logger_unset_stdout(struct logger* logger)
{
    CHECK_IF(logger == NULL, return LOG_FAIL, "logger is null");
    logger->flag &= ~LOG_FLAG_STDOUT;
    return LOG_OK;
}

int logger_set_stdout(struct logger* logger)
{
    CHECK_IF(logger == NULL, return LOG_FAIL, "logger is null");    
    logger->flag |= LOG_FLAG_STDOUT;
    return LOG_OK;
}


struct logger* logger_create(void)
{
    struct logger* lg = calloc(sizeof(struct logger), 1);
    lg->level = LOG_LV_DEFAULT;
    queue_init(&lg->mq, -1, _clean_msg, QUEUE_FLAG_POP_BLOCK);
    return lg;
}

void logger_release(struct logger* lg)
{
    CHECK_IF(lg == NULL, return, "lg is null");

    queue_clean(&lg->mq);

    if (lg->running) logger_break(lg);

    if (lg->flag & LOG_FLAG_UDP) logger_unset_udp(lg);

    if (lg->flag & LOG_FLAG_FILE) logger_unset_file(lg);

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

void logger_set_level(struct logger* lg, int level)
{
    CHECK_IF(lg == NULL, return, "lg is null");
    lg->level = level;
}

int logger_get_level(struct logger* lg)
{
    CHECK_IF(lg == NULL, return LOG_LV_INVALID, "lg is null");
    return lg->level;
}

void logger_enq(struct logger* lg, char* str)
{
    CHECK_IF(lg == NULL, return, "lg is null");
    queue_push(&lg->mq, str);
}
