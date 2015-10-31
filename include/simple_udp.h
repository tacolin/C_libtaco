#ifndef _SIMPLE_UDP_H
#define _SIMPLE_UDP_H

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SUDP_OK (0)
#define SUDP_FAIL (-1)

union ipaddr
{
    struct sockaddr_storage ss;
    struct sockaddr_in s4;
    struct sockaddr_in6 s6;
};

typedef union ipaddr tIpAddr;

int ipaddr_build(char* ipstr, int port, union ipaddr* output_addr);
int ipaddr_parse(union ipaddr addr, char* output_ipstr, int* output_port);

int sudp_open_socket(int family);
int sudp_open_bound_socket(char* my_ipstr, int my_port, union ipaddr* output_bound_addr);
int sudp_open_bound_socket_to_addr(union ipaddr my_addr);
void sudp_close_socket(int* fd);

int sudp_send(int fd, void* data, int data_len, char* dst_ipstr, int dst_port);
int sudp_send_to_addr(int fd, void* data, int data_len, union ipaddr dst_addr);

int sudp_receive(int fd, void* buffer, int buffer_size, union ipaddr* src_addr);
int sudp_receive_timed(int fd, void* buffer, int buffer_size, union ipaddr* src_addr, int timeout_ms);
int sudp_receive_nonblocking(int fd, void* buffer, int buffer_size, union ipaddr* src_addr);

#endif //_SIMPLE_UDP_H
