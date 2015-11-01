#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "udp.h"

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#define LOCK(udp) atom_spinlock(&udp->lock)
#define UNLOCK(udp) atom_spinunlock(&udp->lock)

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

int udp_to_sockaddr(struct udp_addr udp_addr, union sock_addr* sock_addr)
{
    assert(sock_addr != NULL);

    if (1 == inet_pton(AF_INET, udp_addr.ip, &sock_addr->s4.sin_addr))
    {
        sock_addr->ss.ss_family = AF_INET;
        sock_addr->s4.sin_port = htons(udp_addr.port);
    }
    else if (1 == inet_pton(AF_INET6, udp_addr.ip, &sock_addr->s6.sin6_addr))
    {
        sock_addr->ss.ss_family = AF_INET6;
        sock_addr->s6.sin6_port = htons(udp_addr.port);
    }
    else
    {
        derror("inet_pton v4 and v6 failed, ip = %s", udp_addr.ip);
        return UDP_FAIL;
    }
    return UDP_OK;
}

int udp_to_udpaddr(union sock_addr sock_addr, struct udp_addr* udp_addr)
{
    assert(udp_addr != NULL);

    if (sock_addr.ss.ss_family == AF_INET)
    {
        inet_ntop(AF_INET, &sock_addr.s4.sin_addr, udp_addr->ip, INET_ADDRSTRLEN);
        udp_addr->port = ntohs(sock_addr.s4.sin_port);
    }
    else if (sock_addr.ss.ss_family == AF_INET6)
    {
        inet_ntop(AF_INET6, &sock_addr.s6.sin6_addr, udp_addr->ip, INET6_ADDRSTRLEN);
        udp_addr->port = ntohs(sock_addr.s6.sin6_port);
    }
    else
    {
        derror("invalid sin_family = %d", sock_addr.ss.ss_family);
        return UDP_FAIL;
    }
    return UDP_OK;
}

static int _init_udpv4(struct udp* udp, char* local_ip, int local_port)
{
    assert(udp != NULL);

    udp->local_port = local_port;
    udp->fd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_IF(udp->fd < 0, return UDP_FAIL, "socket failed. %s", strerror(errno));

    int chk;
    const int on = 1;

    chk = setsockopt(udp->fd, SOL_SOCKET, SO_BROADCAST, (void*)&on, sizeof(on));
    CHECK_IF(chk < 0, goto _ERROR, "setsockopt broadcast failed. %s", strerror(errno));

    if (udp->local_port != UDP_PORT_ANY)
    {
        struct sockaddr_in me = {};
        me.sin_family = AF_INET;
        if (local_ip)
        {
            chk = inet_pton(AF_INET, local_ip, &me.sin_addr);
            CHECK_IF(chk != 1, goto _ERROR, "inet_pton failed. %s", strerror(errno));
        }
        else
        {
            me.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        me.sin_port = htons(local_port);

        chk = setsockopt(udp->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
        CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuse failed. %s", strerror(errno));

        chk = bind(udp->fd, (struct sockaddr*)&me, sizeof(me));
        CHECK_IF(chk < 0, goto _ERROR, "bind failed. %s", strerror(errno));
    }

    udp->type = UDPv4;
    udp->lock = 0;
    udp->is_init = 1;
    return UDP_OK;

_ERROR:
    if (udp->fd > 0)
    {
        close(udp->fd);
        udp->fd = -1;
    }
    return UDP_FAIL;
}

static int _init_udpv6(struct udp* udp, char* local_ip, int local_port)
{
    assert(udp != NULL);

    udp->local_port = local_port;
    udp->fd = socket(AF_INET6, SOCK_DGRAM, 0);
    CHECK_IF(udp->fd < 0, return UDP_FAIL, "socket failed. %s", strerror(errno));

    int chk;
    const int on = 1;

    chk = setsockopt(udp->fd, SOL_SOCKET, SO_BROADCAST, (void*)&on, sizeof(on));
    CHECK_IF(chk < 0, goto _ERROR, "setsockopt broadcast failed. %s", strerror(errno));

    if (udp->local_port != UDP_PORT_ANY)
    {
        struct sockaddr_in6 me = {};
        me.sin6_family = AF_INET6;
        if (local_ip)
        {
            chk = inet_pton(AF_INET6, local_ip, &me.sin6_addr);
            CHECK_IF(chk != 1, goto _ERROR, "inet_pton failed. %s", strerror(errno));
        }
        else
        {
            me.sin6_addr = in6addr_any;
        }
        me.sin6_port = htons(local_port);

        chk = setsockopt(udp->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
        CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuse failed. %s", strerror(errno));

        chk = bind(udp->fd, (struct sockaddr*)&me, sizeof(me));
        CHECK_IF(chk < 0, goto _ERROR, "bind failed. %s", strerror(errno));
    }

    udp->type = UDPv6;
    udp->lock = 0;
    udp->is_init = 1;
    return UDP_OK;

_ERROR:
    if (udp->fd > 0)
    {
        close(udp->fd);
        udp->fd = -1;
    }
    return UDP_FAIL;
}

int udp_init(struct udp* udp, char* local_ip, int local_port)
{
    assert(udp != NULL);

    if (local_ip)
    {
        struct sockaddr_in me = {};
        if (1 == inet_pton(AF_INET, local_ip, &me.sin_addr))
        {
            return _init_udpv4(udp, local_ip, local_port);
        }
        else
        {
            return _init_udpv6(udp, local_ip, local_port);
        }
    }
    return _init_udpv6(udp, local_ip, local_port);
}

int udp_send(struct udp* udp, struct udp_addr remote, void* data, int data_len)
{
    assert(udp != NULL);
    assert(udp->fd > 0);
    assert(udp->is_init == 1);
    assert(data != NULL);
    assert(data_len > 0);

    union sock_addr remote_addr = {};
    int addrlen = sizeof(remote_addr);

    int chk = udp_to_sockaddr(remote, &remote_addr);
    CHECK_IF(chk != UDP_OK, return -1, "udp_to_sockaddr failed");

    LOCK(udp);
    int sendlen = sendto(udp->fd, data, data_len, 0, (const struct sockaddr*)&remote_addr, addrlen);
    UNLOCK(udp);
    return sendlen;
}

int udp_recv(struct udp* udp, void* buffer, int buffer_size, struct udp_addr* remote)
{
    assert(udp != NULL);
    assert(udp->fd > 0);
    assert(udp->is_init == 1);
    assert(buffer != NULL);
    assert(buffer_size > 0);

    union sock_addr remote_addr = {};
    int addrlen = sizeof(remote_addr);

    int recvlen = recvfrom(udp->fd, buffer, buffer_size, 0, (struct sockaddr*)&remote_addr, &addrlen);
    int chk = udp_to_udpaddr(remote_addr, remote);
    CHECK_IF(chk != UDP_OK, return -1, "udp_to_udpaddr failed");

    return recvlen;
}

int udp_uninit(struct udp* udp)
{
    assert(udp != NULL);
    assert(udp->fd > 0);
    assert(udp->is_init == 1);

    close(udp->fd);
    udp->fd = -1;
    return UDP_OK;
}
