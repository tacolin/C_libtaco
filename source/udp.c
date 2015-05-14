#include <stdio.h>

#include "udp.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

int udp_to_sockaddr(struct udp_addr udp_addr, struct sockaddr_in* sock_addr)
{
    CHECK_IF(sock_addr == NULL, return UDP_FAIL, "sock_addr is null");

    int chk = inet_pton(AF_INET, udp_addr.ipv4, &sock_addr->sin_addr);
    CHECK_IF(chk != 1, return UDP_FAIL, "inet_pton failed");

    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port   = htons(udp_addr.port);
    return UDP_OK;
}

int udp_to_udpaddr(struct sockaddr_in sock_addr, struct udp_addr* udp_addr)
{
    CHECK_IF(udp_addr == NULL, return UDP_FAIL, "udp_addr is null");

    inet_ntop(AF_INET, &sock_addr.sin_addr, udp_addr->ipv4, INET_ADDRSTRLEN);
    udp_addr->port = ntohs(sock_addr.sin_port);
    return UDP_OK;
}

int udp_init(struct udp* udp, char* local_ip, int local_port)
{
    CHECK_IF(udp == NULL, return UDP_FAIL, "udp is null");

    udp->local_port = local_port;
    udp->fd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_IF(udp->fd < 0, return UDP_FAIL, "socket failed");

    int chk;
    const int on = 1;

    chk = setsockopt(udp->fd, SOL_SOCKET, SO_BROADCAST, (void*)&on, sizeof(on));
    CHECK_IF(chk < 0, goto _ERROR, "setsockopt broadcast failed");

    if (udp->local_port != UDP_PORT_ANY)
    {
        struct sockaddr_in me = {};
        me.sin_family = AF_INET;
        if (local_ip)
        {
            chk = inet_pton(AF_INET, local_ip, &me.sin_addr);
            CHECK_IF(chk != 1, goto _ERROR, "inet_pton failed");
        }
        else
        {
            me.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        me.sin_port = htons(local_port);

        chk = setsockopt(udp->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
        CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuse failed");

        chk = bind(udp->fd, (struct sockaddr*)&me, sizeof(me));
        CHECK_IF(chk < 0, goto _ERROR, "bind failed");
    }

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

int udp_send(struct udp* udp, struct udp_addr remote, void* data, int data_len)
{
    CHECK_IF(udp == NULL, return UDP_FAIL, "udp is null");
    CHECK_IF(udp->fd <= 0, return UDP_FAIL, "udp fd <= 0");
    CHECK_IF(udp->is_init != 1, return UDP_FAIL, "udp is not initialized yet");
    CHECK_IF(data == NULL, return UDP_FAIL, "data is null");
    CHECK_IF(data_len <= 0, return UDP_FAIL, "data_len is %d", data_len);

    struct sockaddr_in remote_addr = {};
    int addrlen = sizeof(remote_addr);

    int chk = udp_to_sockaddr(remote, &remote_addr);
    CHECK_IF(chk != UDP_OK, return -1, "udp_to_sockaddr failed");

    return sendto(udp->fd, data, data_len, 0, (const struct sockaddr*)&remote_addr, addrlen);
}

int udp_recv(struct udp* udp, void* buffer, int buffer_size, struct udp_addr* remote)
{
    CHECK_IF(udp == NULL, return UDP_FAIL, "udp is null");
    CHECK_IF(udp->fd <= 0, return UDP_FAIL, "udp fd <= 0");
    CHECK_IF(udp->is_init != 1, return UDP_FAIL, "udp is not initialized yet");
    CHECK_IF(buffer == NULL, return UDP_FAIL, "buffer is null");
    CHECK_IF(buffer_size <= 0, return UDP_FAIL, "buffer_size is %d", buffer_size);

    struct sockaddr_in remote_addr = {};
    int addrlen = sizeof(remote_addr);

    int recvlen = recvfrom(udp->fd, buffer, buffer_size, 0, (struct sockaddr*)&remote_addr, &addrlen);
    int chk = udp_to_udpaddr(remote_addr, remote);
    CHECK_IF(chk != UDP_OK, return -1, "udp_to_udpaddr failed");

    return recvlen;
}

int udp_uninit(struct udp* udp)
{
    CHECK_IF(udp == NULL, return UDP_FAIL, "udp is null");
    CHECK_IF(udp->fd <= 0, return UDP_FAIL, "udp fd <= 0");
    CHECK_IF(udp->is_init != 1, return UDP_FAIL, "udp is not initialized yet");

    close(udp->fd);
    udp->fd = -1;
    return UDP_OK;
}
