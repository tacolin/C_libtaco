#ifndef _UNSOCK_H_
#define _UNSOCK_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define UNPATH_SIZE 108
#define UN_OK (0)
#define UN_FAIL (-1)

struct untcp
{
    int fd;
    char path[UNPATH_SIZE];
    int is_init;
};

struct untcp_server
{
    int fd;
    char path[UNPATH_SIZE];
    int is_init;
};

int untcp_server_init(struct untcp_server* server, char* path, int max_conn_num);
int untcp_server_uninit(struct untcp_server* server);
int untcp_server_accept(struct untcp_server* server, struct untcp* untcp);

int untcp_client_init(struct untcp* untcp, char* path);
int untcp_client_uninit(struct untcp* untcp);

int untcp_recv(struct untcp* untcp, void* buffer, int buffer_size);
int untcp_send(struct untcp* untcp, void* data, int data_len);

#endif //_UNSOCK_H_
