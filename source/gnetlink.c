#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gnetlink.h"

#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#define GNMSG_OFS (sizeof(struct nlmsghdr) + sizeof(struct genlmsghdr) + sizeof(struct nlattr))

struct gnmsg
{
    struct nlmsghdr n;
    struct genlmsghdr g;
    struct nlattr na;
};

static int _create_gnlsock(int groups)
{
    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    CHECK_IF(fd < 0, return -1, "socket failed");

    struct sockaddr_nl addr = {};
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = groups;
    int chk = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    CHECK_IF(chk < 0, goto _ERROR, "bind failed");

    return fd;

_ERROR:
    close(fd);
    return -1;
}

static uint16_t _get_familyid(int fd, char* name)
{
    CHECK_IF(fd < 0, return 0, "fd < 0");
    CHECK_IF(name == NULL, return 0, "name is null");

    struct gnmsg *msg  = calloc(sizeof(struct gnmsg) + 256, 1);
    msg->n.nlmsg_type  = GENL_ID_CTRL;
    msg->n.nlmsg_flags = NLM_F_REQUEST;
    msg->n.nlmsg_seq   = 0;
    msg->n.nlmsg_pid   = getpid();
    msg->n.nlmsg_len   = NLMSG_LENGTH(GENL_HDRLEN);

    msg->g.cmd     = CTRL_CMD_GETFAMILY;
    msg->g.version = 0x1;

    msg->na.nla_type = CTRL_ATTR_FAMILY_NAME;
    msg->na.nla_len  = strlen(name) + 1 + NLA_HDRLEN;
    memcpy(NLA_DATA(&msg->na), name, strlen(name)+1);

    msg->n.nlmsg_len += NLMSG_ALIGN(msg->na.nla_len);

    struct sockaddr_nl addr = {.nl_family = AF_NETLINK};
    int chk = sendto(fd, msg, msg->n.nlmsg_len, 0, (struct sockaddr*)&addr, sizeof(addr));
    CHECK_IF(chk < 0, goto _ERROR, "sendto failed");

    free(msg);

    msg = calloc(sizeof(struct gnmsg) + 256, 1);
    int recvlen = recv(fd, msg, sizeof(struct gnmsg) + 256, 0);
    CHECK_IF(recvlen < 0, goto _ERROR, "recv failed");
    CHECK_IF(!NLMSG_OK(&msg->n, recvlen), goto _ERROR, "invalid recv msg");
    CHECK_IF(msg->n.nlmsg_type == NLMSG_ERROR, goto _ERROR, "recv error msg");

    void* ptr = &(msg->na);
    ptr       += NLA_ALIGN(msg->na.nla_len);

    struct nlattr* na = (struct nlattr*)ptr;
    CHECK_IF(na->nla_type != CTRL_ATTR_FAMILY_ID, goto _ERROR, "get no family id");

    int id = *(uint16_t*)NLA_DATA(na);

    free(msg);

    return id;

_ERROR:
    if (msg) free(msg);

    return 0;
}

int gnetlink_init(struct gnetlink* gnl, char* kmod_name, int groups)
{
    CHECK_IF(gnl == NULL, return GNL_FAIL, "gnl is null");
    CHECK_IF(kmod_name == NULL, return GNL_FAIL, "kmod_name is null");

    gnl->fd = _create_gnlsock(groups);
    CHECK_IF(gnl->fd < 0, return GNL_FAIL, "_create_gnlsock failed");

    gnl->familyid = _get_familyid(gnl->fd, kmod_name);
    CHECK_IF(gnl->familyid == 0, goto _ERROR, "_get_familyid failed");

    gnl->is_init = true;
    return GNL_OK;

_ERROR:
    close(gnl->fd);
    return GNL_FAIL;
}

void gnetlink_uninit(struct gnetlink* gnl)
{
    CHECK_IF(gnl == NULL, return, "gnl is null");
    CHECK_IF(gnl->is_init == false, return, "gnl is not init yet");
    CHECK_IF(gnl->fd <= 0, return, "gnl->fd = %d invalid", gnl->fd);

    close(gnl->fd);
    gnl->fd = -1;
    gnl->is_init = false;
    return;
}

int gnetlink_send(struct gnetlink* gnl, uint16_t seq, uint8_t cmd, void* data, int datalen)
{
    CHECK_IF(gnl == NULL, return -1, "gnl is null");
    CHECK_IF(gnl->is_init == false, return -1, "gnl is not init yet");
    CHECK_IF(gnl->fd <= 0, return -1, "gnl->fd = %d invalid", gnl->fd);
    CHECK_IF(data == NULL, return -1, "data is null");
    CHECK_IF(datalen <= 0, return -1, "datalen = %d invalid", datalen);

    struct gnmsg* msg  = calloc(sizeof(struct gnmsg) + datalen, 1);
    msg->n.nlmsg_len   = NLMSG_LENGTH(GENL_HDRLEN);
    msg->n.nlmsg_type  = gnl->familyid;
    msg->n.nlmsg_flags = NLM_F_REQUEST;
    msg->n.nlmsg_seq   = seq;
    msg->n.nlmsg_pid   = getpid();

    msg->g.cmd = cmd;

    msg->na.nla_type = 1;
    msg->na.nla_len  = datalen + NLA_HDRLEN;
    memcpy((void*)NLA_DATA(&msg->na), data, datalen);

    msg->n.nlmsg_len += NLMSG_ALIGN(msg->na.nla_len);

    struct sockaddr_nl addr = {};
    addr.nl_family = AF_NETLINK;

    int sendlen = sendto(gnl->fd, msg, msg->n.nlmsg_len, 0, (struct sockaddr*)&addr, sizeof(addr));

    free(msg);

    return sendlen;
}

int gnetlink_recv(struct gnetlink* gnl, void* buffer, int bufsize)
{
    CHECK_IF(gnl == NULL, return -1, "gnl is null");
    CHECK_IF(gnl->is_init == false, return -1, "gnl is not init yet");
    CHECK_IF(gnl->fd <= 0, return -1, "gnl->fd = %d invalid", gnl->fd);
    CHECK_IF(buffer == NULL, return -1, "buffer is null");
    CHECK_IF(bufsize <= 0, return -1, "bufsize = %d invalid", bufsize);

    int recvlen = recv(gnl->fd, buffer, bufsize, 0);
    struct gnmsg* msg = (struct gnmsg*)buffer;
    CHECK_IF(recvlen < 0, return -1, "recv failed");
    CHECK_IF(!NLMSG_OK(&msg->n, recvlen), return -1, "invalid recv msg");
    CHECK_IF(msg->n.nlmsg_type == NLMSG_ERROR, return -1, "recv error msg");

    return recvlen;
}

int gnetlink_getdata(void* origin, int originlen, void** data, int *datalen)
{
    CHECK_IF(origin == NULL, return GNL_FAIL, "origin is null");
    CHECK_IF(originlen <= 0, return GNL_FAIL, "originlen = %d invalid", originlen);
    CHECK_IF(data == NULL, return GNL_FAIL, "data is null");
    CHECK_IF(datalen == NULL, return GNL_FAIL, "datalen is null");

    struct nlattr* na = (struct nlattr*)GENLMSG_DATA(origin);
    *data = (void*)NLA_DATA(na);
    *datalen = na->nla_len;
    return GNL_OK;
}
