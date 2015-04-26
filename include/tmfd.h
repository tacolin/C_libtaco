#ifndef _TMFD_H_
#define _TMFD_H_

////////////////////////////////////////////////////////////////////////////////

#define TMFD_TICK_MS (20)

#define TMFD_CLOCK_MONOTONIC (1)

#define TMFD_OK   (0)
#define TMFD_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

struct tmfdspec
{
    long tv_sec;
    long tv_nsec;
};

struct itmfdspec
{
    struct tmfdspec it_interval;
    struct tmfdspec it_value;
};

int tmfd_create(int clockid, int flags);
int tmfd_settime(int tmfd, int flags, const struct itmfdspec *new_value, struct itmfdspec *old_value);
int tmfd_gettime(int tmfd, struct itmfdspec *old_value);
void tmfd_close(int tmfd);

int tmfd_system_init(void);
void tmfd_system_uninit(void);

#endif //_TMFD_H_