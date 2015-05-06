#include <stdio.h>

#include "unsock.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

int untcp_server_init(struct untcp_server* server, char* path, int max_conn_num)
{
    CHECK_IF(server == NULL, return UN_FAIL, "server is null");
    CHECK_IF(path == NULL, return UN_FAIL, "path is null");
    CHECK_IF(max_conn_num <= 0, return UN_FAIL, "max_conn_num = %d", max_conn_num);

    server->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    CHECK_IF(server->fd < 0, return UN_FAIL, "socket failed");

    snprintf(server->path, UNPATH_SIZE, "%s", path);
    unlink(path);

    struct sockaddr_un me = {};
    me.sun_family = AF_UNIX;
    snprintf(me.sun_path, 108, "%s", server->path);

    int len = strlen(me.sun_path) + sizeof(me.sun_family);
    int chk = bind(server->fd,(struct sockaddr*)&me, len);
    CHECK_IF(chk < 0, goto _ERROR, "bind failed");

    chk = listen(server->fd, max_conn_num);
    CHECK_IF(chk < 0, goto _ERROR, "listen failed");

    server->is_init = 1;
    return UN_OK;

_ERROR:
    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }
    return UN_FAIL;
}

int untcp_server_uninit(struct untcp_server* server)
{
    CHECK_IF(server == NULL, return UN_FAIL, "server is null");
    CHECK_IF(server->is_init != 1, return UN_FAIL, "server is not init yet");

    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }
    unlink(server->path);
    server->is_init = 0;
    return UN_OK;
}

int untcp_server_accept(struct untcp_server* server, struct untcp* untcp)
{
    CHECK_IF(server == NULL, return UN_FAIL, "server is null");
    CHECK_IF(server->is_init != 1, return UN_FAIL, "server is not init yet");
    CHECK_IF(server->fd <= 0, return UN_FAIL, "server fd = %d <= 0", server->fd);
    CHECK_IF(untcp == NULL, return UN_FAIL, "untcp is null");

    struct sockaddr_un remote = {};
    int addrlen = sizeof(remote);

    untcp->fd = accept(server->fd, (struct sockaddr*)&remote, &addrlen);
    CHECK_IF(untcp->fd < 0, return UN_FAIL, "accept failed");

    snprintf(untcp->path, UNPATH_SIZE, "%s", server->path);
    untcp->is_init = 1;
    return UN_OK;
}

int untcp_client_init(struct untcp* untcp, char* path)
{
    CHECK_IF(untcp == NULL, return UN_FAIL, "untcp is null");
    CHECK_IF(path == NULL, return UN_FAIL, "path is null");

    untcp->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    CHECK_IF(untcp->fd < 0, return UN_FAIL, "socket failed");

    struct sockaddr_un remote = {};
    remote.sun_family = AF_UNIX;
    snprintf(remote.sun_path, 108, "%s", path);
    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    int chk = connect(untcp->fd, (struct sockaddr*)&remote, len);
    CHECK_IF(chk < 0, goto _ERROR, "connect failed");

    snprintf(untcp->path, UNPATH_SIZE, "%s", path);
    untcp->is_init = 1;
    return UN_OK;

_ERROR:
    if (untcp->fd > 0)
    {
        close(untcp->fd);
        untcp->fd = -1;
    }
    return UN_FAIL;
}

int untcp_client_uninit(struct untcp* untcp)
{
    CHECK_IF(untcp == NULL, return UN_FAIL, "untcp is null");
    CHECK_IF(untcp->is_init != 1, return UN_FAIL, "untcp is not init yet");

    if (untcp->fd > 0)
    {
        close(untcp->fd);
        untcp->fd = -1;
    }
    untcp->is_init = 0;
    return UN_OK;
}

int untcp_recv(struct untcp* untcp, void* buffer, int buffer_size)
{
    CHECK_IF(untcp == NULL, return UN_FAIL, "untcp is null");
    CHECK_IF(untcp->is_init != 1, return UN_FAIL, "untcp is not init yet");
    CHECK_IF(untcp->fd <= 0, return UN_FAIL, "untcp fd (%d) <= 0", untcp->fd);
    CHECK_IF(buffer == NULL, return UN_FAIL, "buffer is null");
    CHECK_IF(buffer_size <= 0, return UN_FAIL, "buffer_size (%d) <= 0", buffer_size);
    return recv(untcp->fd, buffer, buffer_size, 0);
}

int untcp_send(struct untcp* untcp, void* data, int data_len)
{
    CHECK_IF(untcp == NULL, return UN_FAIL, "untcp is null");
    CHECK_IF(untcp->is_init != 1, return UN_FAIL, "untcp is not init yet");
    CHECK_IF(untcp->fd <= 0, return UN_FAIL, "untcp fd (%d) <= 0", untcp->fd);
    CHECK_IF(data == NULL, return UN_FAIL, "data is null");
    CHECK_IF(data_len <= 0, return UN_FAIL, "data_len (%d) <= 0", data_len);
    return send(untcp->fd, data, data_len, 0);
}
