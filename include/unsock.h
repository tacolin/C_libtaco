#ifndef _UNSOCK_H_
#define _UNSOCK_H_

#include "basic.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

////////////////////////////////////////////////////////////////////////////////

#define UNPATH_SIZE 108

////////////////////////////////////////////////////////////////////////////////

typedef enum 
{
    UN_OK = 0,
    UN_ERROR = -1,

} tUnStatus;

typedef struct tUnTcp
{
    int fd;
    char path[UNPATH_SIZE];
    int is_init;

} tUnTcp;

typedef struct tUnTcpServer
{
    int fd;
    char path[UNPATH_SIZE];
    int is_init;

} tUnTcpServer;

////////////////////////////////////////////////////////////////////////////////

tUnStatus untcp_server_init(tUnTcpServer* server, char* path, int max_conn_num);
tUnStatus untcp_server_uninit(tUnTcpServer* server);
tUnStatus untcp_server_accept(tUnTcpServer* server, tUnTcp* untcp);

tUnStatus untcp_client_init(tUnTcp* untcp, char* path);
tUnStatus untcp_client_uninit(tUnTcp* untcp);

int untcp_recv(tUnTcp* untcp, void* buffer, int buffer_size);
int untcp_send(tUnTcp* untcp, void* data, int data_len);


#endif //_UNSOCK_H_