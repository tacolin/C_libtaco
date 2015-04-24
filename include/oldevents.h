#ifndef _OLD_EVENTS_H_
#define _OLD_EVENTS_H_

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////

#define FPOLL_CTL_ADD  (1)
#define FPOLL_CTL_DEL  (2)

#define FPOLLIN (1)

typedef struct fpoll_data
{
    void* ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;

} fpoll_data_t;

struct fpoll_event
{
    uint32_t events;
    struct fpoll_data data;
};


int fpoll_create(int max);
void fpoll_close(int fpd);

int fpoll_wait(int fpd, struct fpoll_event *events, int maxevents, int timeout);
int fpoll_ctl(int fpd, int op, int fd, struct fpoll_event *event);

int fpoll_init(void);
void fpoll_uninit(void);

////////////////////////////////////////////////////////////////////////////////

#define TM_TICK_MS (10)
#define TM_CLOCK_MONOTONIC (1)

struct tmspec
{
    long tv_sec;
    long tv_nsec;
};

struct itmspec
{
    struct tmspec it_interval;
    struct tmspec it_value;
};

int tmfd_create(int clockid, int flags);
int tmfd_settime(int tmfd, int flags, const struct itmspec *new_value, struct itmspec *old_value);
int tmfd_gettime(int tmfd, struct itmspec *old_value);
void tmfd_close(int tmfd);

int tmfd_init(void);
void tmfd_uninit(void);

#endif //_OLD_EVENTS_H_
