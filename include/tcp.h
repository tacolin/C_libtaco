#ifndef _TCP_H_
#define _TCP_H_

#include "basic.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

////////////////////////////////////////////////////////////////////////////////

#define TCP_PORT_ANY -1

#define TCP_OK (0)
#define TCP_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef struct tTcpAddr
{
    char ipv4[INET_ADDRSTRLEN];
    int  port;

} tTcpAddr;

typedef struct tTcp
{
    int fd;
    int local_port;
    int is_init;

    tTcpAddr remote;

} tTcp;

typedef struct tTcpServer
{
    int fd;
    int local_port;
    int  is_init;

} tTcpServer;

////////////////////////////////////////////////////////////////////////////////

int tcp_server_init(tTcpServer* server, char* local_ip, int local_port, int max_conn_num);
int tcp_server_uninit(tTcpServer* server);
int tcp_server_accept(tTcpServer* server, tTcp* tcp);

int tcp_client_init(tTcp* tcp, char* remote_ip, int remote_port, int local_port);
int tcp_client_uninit(tTcp* tcp);

int tcp_recv(tTcp* tcp, void* buffer, int buffer_size);
int tcp_send(tTcp* tcp, void* data, int data_len);

#endif //_TCP_H_
