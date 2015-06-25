#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <sys/time.h>

#include "udp.h"
#include "queue.h"

#define LOG_OK (0)
#define LOG_FAIL (-1)

#define LOG_LV_DEFAULT LOG_LV_INFO

#define log_print(logger, level, msg, param...)\
{\
    if (logger_getlevel(logger) >= level)\
    {\
        struct timeval __tv;\
        char* __tmp;\
        gettimeofday(&__tv, NULL);\
        asprintf(&__tmp, "(%03lds,%03ldms): "msg, __tv.tv_sec%1000, __tv.tv_usec/1000, ##param);\
        logger_enq(logger, __tmp);\
    }\
}

// #define log_print(logger, level, msg, param...)\
// {\
//     if (logger_getlevel(logger) >= level)\
//     {\
//         struct timeval __tv;\
//         char* __tmp;\
//         gettimeofday(&__tv, NULL);\
//         asprintf(&__tmp, "(%03lds,%03ldms): "msg, __tv.tv_sec%1000, __tv.tv_usec/1000, ##param);\
//         logger_handlemsg(logger, __tmp);\
//         free(__tmp);
//     }\
// }

enum
{
    LOG_FLAG_STDOUT = 0x1,
    LOG_FLAG_UDP    = 0x2,
    LOG_FLAG_FILE   = 0x4
};

enum
{
    LOG_LV_INVALID = -1,
    LOG_LV_FATAL = 0,
    LOG_LV_ERR,
    LOG_LV_WARN,
    LOG_LV_INFO,
    LOG_LV_V1,
    LOG_LV_V2,
    LOG_LV_V3,

    LOG_LEVELS
};

struct logger;

struct logger* logger_create(int flag, char* dstip, int dstport, char* filepath);
void logger_release(struct logger* lg);

void logger_run(struct logger* lg);
void logger_break(struct logger* lg);

void logger_setlevel(struct logger* lg, int lv);
int logger_getlevel(struct logger* lg);

void logger_enq(struct logger* lg, char* str);

int logger_set_udp(struct logger* logger, char* dstip, int dstport);
int logger_unset_udp(struct logger* logger);

int logger_set_file(struct logger* logger, char* filepath);
int logger_unset_file(struct logger* logger);

#endif //_LOGGER_H_
