#include "udp.h"

////////////////////////////////////////////////////////////////////////////////

int udp_toSockAddr(tUdpAddr udp_addr, struct sockaddr_in* sock_addr)
{
    check_if(sock_addr == NULL, return UDP_FAIL, "sock_addr is null");

    sock_addr->sin_family = AF_INET;

    int check = inet_pton(AF_INET, udp_addr.ipv4, &sock_addr->sin_addr);
    check_if(check != 1, return UDP_FAIL, "inet_pton failed");

    sock_addr->sin_port = htons(udp_addr.port);

    return UDP_OK;
}

int udp_toUdpAddr(struct sockaddr_in sock_addr, tUdpAddr* udp_addr)
{
    check_if(udp_addr == NULL, return UDP_FAIL, "udp_addr is null");

    inet_ntop(AF_INET, &sock_addr.sin_addr, udp_addr->ipv4, INET_ADDRSTRLEN);

    udp_addr->port = ntohs(sock_addr.sin_port);

    return UDP_OK;
}

int udp_init(tUdp* udp, char* local_ip, int local_port)
{
    check_if(udp == NULL, return UDP_FAIL, "udp is null");

    udp->local_port = local_port;

    udp->fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_if(udp->fd < 0, return UDP_FAIL, "socket failed");

    int check;

    if (udp->local_port != UDP_PORT_ANY)
    {
        struct sockaddr_in me = {};
        me.sin_family = AF_INET;
        if (local_ip)
        {
            check = inet_pton(AF_INET, local_ip, &me.sin_addr);
            check_if(check != 1, goto _ERROR, "inet_pton failed");
        }
        else
        {
            me.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        me.sin_port = htons(local_port);

        const int on = 1;
        check = setsockopt(udp->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
        check_if(check < 0, goto _ERROR, "setsockopt reuse failed");

        check = bind(udp->fd, (struct sockaddr*)&me, sizeof(me));
        check_if(check < 0, goto _ERROR, "bind failed");
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

int udp_send(tUdp* udp, tUdpAddr remote, void* data, int data_len)
{
    check_if(udp == NULL, return UDP_FAIL, "udp is null");
    check_if(udp->fd <= 0, return UDP_FAIL, "udp fd <= 0");
    check_if(udp->is_init != 1, return UDP_FAIL, "udp is not initialized yet");

    check_if(data == NULL, return UDP_FAIL, "data is null");
    check_if(data_len <= 0, return UDP_FAIL, "data_len is %d", data_len);

    struct sockaddr_in remote_addr = {};
    int addrlen = sizeof(remote_addr);

    int check = udp_toSockAddr(remote, &remote_addr);
    check_if(check != UDP_OK, return -1, "udp_toSockAddr failed");

    return sendto(udp->fd, data, data_len, 0,
                  (const struct sockaddr*)&remote_addr, addrlen);

}

int udp_recv(tUdp* udp, void* buffer, int buffer_size, tUdpAddr* remote)
{
    check_if(udp == NULL, return UDP_FAIL, "udp is null");
    check_if(udp->fd <= 0, return UDP_FAIL, "udp fd <= 0");
    check_if(udp->is_init != 1, return UDP_FAIL, "udp is not initialized yet");

    check_if(buffer == NULL, return UDP_FAIL, "buffer is null");
    check_if(buffer_size <= 0, return UDP_FAIL, "buffer_size is %d", buffer_size);

    struct sockaddr_in remote_addr = {};
    int addrlen = sizeof(remote_addr);

    int recvlen = recvfrom(udp->fd, buffer, buffer_size, 0,
                           (struct sockaddr*)&remote_addr, &addrlen);

    int check = udp_toUdpAddr(remote_addr, remote);
    check_if(check != UDP_OK, return -1, "udp_toUdpAddr failed");

    return recvlen;
}

int udp_uninit(tUdp* udp)
{
    check_if(udp == NULL, return UDP_FAIL, "udp is null");
    check_if(udp->fd <= 0, return UDP_FAIL, "udp fd <= 0");
    check_if(udp->is_init != 1, return UDP_FAIL, "udp is not initialized yet");

    close(udp->fd);
    udp->fd = -1;

    return UDP_OK;
}

