#ifndef _UDP_H_
#define _UDP_H_

#include "basic.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

////////////////////////////////////////////////////////////////////////////////

#define UDP_PORT_ANY -1

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    UDP_OK = 0,
    UDP_ERROR = -1

} tUdpStatus;

typedef struct tUdpAddr
{
    char ipv4[INET_ADDRSTRLEN];
    int  port;

} tUdpAddr;

typedef struct tUdp
{
    int local_port;
    int fd;
    int is_init;

} tUdp;

////////////////////////////////////////////////////////////////////////////////

tUdpStatus udp_init(tUdp* udp, char* local_ip, int local_port);

int udp_send(tUdp* udp, tUdpAddr remote,
                    void* data, int data_len);

int udp_recv(tUdp* udp, void* buffer, int buffer_size, tUdpAddr* remote);

tUdpStatus udp_uninit(tUdp* udp);

tUdpStatus udp_toUdpAddr(struct sockaddr_in sock_addr, tUdpAddr* udp_addr);
tUdpStatus udp_toSockAddr(tUdpAddr udp_addr, struct sockaddr_in* sock_addr);

#endif //_UDP_H_