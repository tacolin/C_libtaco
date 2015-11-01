#ifndef _UDP_H_
#define _UDP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define UDP_PORT_ANY -1
#define UDP_OK (0)
#define UDP_FAIL (-1)

struct udp_addr
{
    char ip[INET_ADDRSTRLEN];
    int  port;
};

struct udp
{
    int local_port;
    int fd;
    int is_init;
};

int udp_send(struct udp* udp, struct udp_addr remote, void* data, int data_len);
int udp_recv(struct udp* udp, void* buffer, int buffer_size, struct udp_addr* remote);

int udp_init(struct udp* udp, char* local_ip, int local_port);
int udp_uninit(struct udp* udp);

int udp_to_udpaddr(struct sockaddr_in sock_addr, struct udp_addr* udp_addr);
int udp_to_sockaddr(struct udp_addr udp_addr, struct sockaddr_in* sock_addr);

#endif //_UDP_H_
