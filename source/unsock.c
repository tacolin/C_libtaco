#include "unsock.h"

tUnStatus untcp_server_init(tUnTcpServer* server, char* path, int max_conn_num)
{
    check_if(server == NULL, return UN_ERROR, "server is null");
    check_if(path == NULL, return UN_ERROR, "path is null");
    check_if(max_conn_num <= 0, return UN_ERROR, "max_conn_num = %d", max_conn_num);

    server->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    check_if(server->fd < 0, return UN_ERROR, "socket failed");

    snprintf(server->path, UNPATH_SIZE, "%s", path);
    unlink(path);

    struct sockaddr_un me = {};
    me.sun_family = AF_UNIX;
    snprintf(me.sun_path, 108, "%s", server->path);

    int len = strlen(me.sun_path) + sizeof(me.sun_family);
    int check = bind(server->fd,(struct sockaddr*)&me, len);
    check_if(check < 0, goto _ERROR, "bind failed");

    check = listen(server->fd, max_conn_num);
    check_if(check < 0, goto _ERROR, "listen failed");

    server->is_init = 1;

    return UN_OK;

_ERROR:
    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }

    return UN_ERROR;
}

tUnStatus untcp_server_uninit(tUnTcpServer* server)
{
    check_if(server == NULL, return UN_ERROR, "server is null");
    check_if(server->is_init != 1, return UN_ERROR, "server is not init yet");

    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }

    unlink(server->path);

    server->is_init = 0;

    return UN_OK;
}

tUnStatus untcp_server_accept(tUnTcpServer* server, tUnTcp* untcp)
{
    check_if(server == NULL, return UN_ERROR, "server is null");
    check_if(server->is_init != 1, return UN_ERROR, "server is not init yet");
    check_if(server->fd <= 0, return UN_ERROR, "server fd = %d <= 0", server->fd);
    check_if(untcp == NULL, return UN_ERROR, "untcp is null");

    struct sockaddr_un remote = {};
    int addrlen = sizeof(remote);

    untcp->fd = accept(server->fd, (struct sockaddr*)&remote, &addrlen);
    check_if(untcp->fd < 0, return UN_ERROR, "accept failed");

    snprintf(untcp->path, UNPATH_SIZE, "%s", server->path);
    untcp->is_init = 1;

    return UN_OK;
}

tUnStatus untcp_client_init(tUnTcp* untcp, char* path)
{
    check_if(untcp == NULL, return UN_ERROR, "untcp is null");
    check_if(path == NULL, return UN_ERROR, "path is null");

    untcp->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    check_if(untcp->fd < 0, return UN_ERROR, "socket failed");

    struct sockaddr_un remote = {};
    remote.sun_family = AF_UNIX;
    snprintf(remote.sun_path, 108, "%s", path);
    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    int check = connect(untcp->fd, (struct sockaddr*)&remote, len);
    check_if(check < 0, goto _ERROR, "connect failed");

    snprintf(untcp->path, UNPATH_SIZE, "%s", path);
    untcp->is_init = 1;

    return UN_OK;

_ERROR:
    if (untcp->fd > 0)
    {
        close(untcp->fd);
        untcp->fd = -1;
    }
    return UN_ERROR;
}

tUnStatus untcp_client_uninit(tUnTcp* untcp)
{
    check_if(untcp == NULL, return UN_ERROR, "untcp is null");
    check_if(untcp->is_init != 1, return UN_ERROR, "untcp is not init yet");

    if (untcp->fd > 0)
    {
        close(untcp->fd);
        untcp->fd = -1;
    }

    untcp->is_init = 0;

    return UN_OK;
}

int untcp_recv(tUnTcp* untcp, void* buffer, int buffer_size)
{
    check_if(untcp == NULL, return UN_ERROR, "untcp is null");
    check_if(untcp->is_init != 1, return UN_ERROR, "untcp is not init yet");
    check_if(untcp->fd <= 0, return UN_ERROR, "untcp fd (%d) <= 0", untcp->fd);
    check_if(buffer == NULL, return UN_ERROR, "buffer is null");
    check_if(buffer_size <= 0, return UN_ERROR, "buffer_size (%d) <= 0", buffer_size);

    return recv(untcp->fd, buffer, buffer_size, 0);
}

int untcp_send(tUnTcp* untcp, void* data, int data_len)
{
    check_if(untcp == NULL, return UN_ERROR, "untcp is null");
    check_if(untcp->is_init != 1, return UN_ERROR, "untcp is not init yet");
    check_if(untcp->fd <= 0, return UN_ERROR, "untcp fd (%d) <= 0", untcp->fd);
    check_if(data == NULL, return UN_ERROR, "data is null");
    check_if(data_len <= 0, return UN_ERROR, "data_len (%d) <= 0", data_len);

    return send(untcp->fd, data, data_len, 0);
}
