#ifndef _GNETLINK_H_
#define _GNETLINK_H_

#include <sys/socket.h>
#include <stdbool.h>
#include <stdint.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <sys/socket.h>
#include <sys/types.h>

#define GNL_OK (0)
#define GNL_FAIL (-1)

struct gnetlink
{
    int fd;
    int nl_groups;
    uint16_t familyid;
    bool is_init;
};

int gnetlink_init(struct gnetlink* gnl, char* kmod_name, int groups);
void gnetlink_uninit(struct gnetlink* gnl);

int gnetlink_send(struct gnetlink* gnl, uint16_t seq, uint8_t cmd, void* data, int datalen);
int gnetlink_recv(struct gnetlink* gnl, void* buffer, int bufsize);

int gnetlink_getdata(void* origin, int originlen, void** data, int *datalen);

#endif //_GNETLINK_H_
