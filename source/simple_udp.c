#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "simple_udp.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

int ipaddr_build(char* ipstr, int port, union ipaddr* output_addr)
{
    assert(output_addr != NULL);

    if (ipstr == NULL)
    {
        output_addr->ss.ss_family = AF_INET;
        output_addr->s4.sin_addr.s_addr = htonl(INADDR_ANY);
        output_addr->s4.sin_port = htons(port);
    }
    else
    {
        if (1 == inet_pton(AF_INET, ipstr, &output_addr->s4.sin_addr))
        {
            output_addr->ss.ss_family = AF_INET;
            output_addr->s4.sin_port = htons(port);
        }
        else if (1 == inet_pton(AF_INET6, ipstr, &output_addr->s6.sin6_addr))
        {
            output_addr->ss.ss_family = AF_INET6;
            output_addr->s6.sin6_port = htons(port);
        }
        else
        {
            return SUDP_FAIL;
        }
    }
    return SUDP_OK;
}

int ipaddr_parse(union ipaddr addr, char* output_ipstr, int* output_port)
{
    assert(output_ipstr != NULL);
    assert(output_port != NULL);

    if (addr.ss.ss_family == AF_INET)
    {
        inet_ntop(AF_INET, &addr.s4.sin_addr, output_ipstr, INET_ADDRSTRLEN);
        *output_port = ntohs(addr.s4.sin_port);
    }
    else if (addr.ss.ss_family == AF_INET6)
    {
        inet_ntop(AF_INET6, &addr.s6.sin6_addr, output_ipstr, INET6_ADDRSTRLEN);
        *output_port = ntohs(addr.s6.sin6_port);
    }
    else
    {
        return SUDP_FAIL;
    }
    return SUDP_OK;
}

int sudp_open_socket(int family)
{
    int fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
    CHECK_IF(fd <= 0, return fd, "socket failed, err = %s", strerror(errno));
    return fd;
}

int sudp_open_bound_socket_to_addr(union ipaddr my_addr)
{
    int fd = sudp_open_socket(my_addr.ss.ss_family);
    CHECK_IF(fd <= 0, goto _ERROR, "udp_open_socket failed");

    const int on = 1;
    int chk = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void*)&on, sizeof(on));
    CHECK_IF(chk < 0, goto _ERROR, "setsockopt broadcast failed, err = %s", strerror(errno));

    chk = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
    CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuse addr failed, err = %s", strerror(errno));

    chk = bind(fd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    CHECK_IF(chk < 0, goto _ERROR, "bind failed, err = %s", strerror(errno));

    return fd;

_ERROR:
    if (fd > 0)
    {
        sudp_close_socket(&fd);
    }
    return -1;
}

int sudp_open_bound_socket(char* my_ipstr, int my_port, union ipaddr* output_bound_addr)
{
    assert(output_bound_addr != NULL);

    union ipaddr addr = {};
    int fd = -1;
    int chk = ipaddr_build(my_ipstr, my_port, &addr);
    CHECK_IF(chk != SUDP_OK, return -1, "ipaddr_build by %s port %d failed", my_ipstr, my_port);

    *output_bound_addr = addr;
    return sudp_open_bound_socket_to_addr(addr);
}

void sudp_close_socket(int* fd)
{
    assert(fd != NULL);
    close(*fd);
    *fd = -1;
}

int sudp_send_to_addr(int fd, void* data, int data_len, union ipaddr dst_addr)
{
    assert(fd > 0);
    assert(data != NULL);
    assert(data_len > 0);

    return sendto(fd, data, data_len, 0, (const struct sockaddr*)&dst_addr, sizeof(dst_addr));
}

int sudp_send(int fd, void* data, int data_len, char* dst_ipstr, int dst_port)
{
    assert(fd > 0);
    assert(data != NULL);
    assert(data_len > 0);
    assert(dst_ipstr != NULL);

    union ipaddr addr = {};
    int chk = ipaddr_build(dst_ipstr, dst_port, &addr);
    CHECK_IF(chk != SUDP_OK, return -1, "ipaddr_build by %s port %d failed", dst_ipstr, dst_port);

    return sudp_send_to_addr(fd, data, data_len, addr);
}

int sudp_receive(int fd, void* buffer, int buffer_size, union ipaddr* src_addr)
{
    assert(fd > 0);
    assert(buffer != NULL);
    assert(buffer_size > 0);
    assert(src_addr != NULL);

    socklen_t addrlen = sizeof(*src_addr);
    int recvlen = recvfrom(fd, buffer, buffer_size, 0, (struct sockaddr*)src_addr, &addrlen);
    CHECK_IF(recvlen < 0, return recvlen, "recvfrom failed, err = %s", strerror(errno));

    return recvlen;
}

int sudp_receive_nonblocking(int fd, void* buffer, int buffer_size, union ipaddr* src_addr)
{
    assert(fd > 0);
    assert(buffer != NULL);
    assert(buffer_size > 0);
    assert(src_addr != NULL);

    socklen_t addrlen = sizeof(*src_addr);
    int recvlen = recvfrom(fd, buffer, buffer_size, MSG_DONTWAIT, (struct sockaddr*)src_addr, &addrlen);
    CHECK_IF(recvlen < 0, return recvlen, "recvfrom failed, err = %s", strerror(errno));

    return recvlen;
}

int sudp_receive_timed(int fd, void* buffer, int buffer_size, union ipaddr* src_addr, int timeout_ms)
{
    assert(fd > 0);
    assert(buffer != NULL);
    assert(buffer_size > 0);
    assert(src_addr != NULL);
    assert(timeout_ms > 0);

    struct timeval timeout =
    {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    int recvlen = 0;
    int ready_num = select(fd+1, &fds, 0, 0, &timeout);
    if (ready_num == 1)
    {
        recvlen = sudp_receive(fd, buffer, buffer_size, src_addr);
    }
    return recvlen;
}
