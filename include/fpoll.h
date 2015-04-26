#ifndef _FPOLL_H_
#define _FPOLL_H_

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////

#define FPOLL_CTL_ADD  (1)
#define FPOLL_CTL_DEL  (2)

#define FPOLLIN (1)

#define FPOLL_OK (0)
#define FPOLL_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef struct fpoll_data
{
    int fd;
    void* ptr;
    uint32_t u32;
    uint64_t u64;

} fpoll_data_t;

struct fpoll_event
{
    uint32_t events;
    struct fpoll_data data;
};

////////////////////////////////////////////////////////////////////////////////

int fpoll_create(int max);
void fpoll_close(int fpd);

int fpoll_wait(int fpd, struct fpoll_event *events, int maxevents, int timeout);
int fpoll_ctl(int fpd, int op, int fd, struct fpoll_event *event);

int fpoll_system_init(void);
void fpoll_system_uninit(void);

#endif //_FPOLL_H_
