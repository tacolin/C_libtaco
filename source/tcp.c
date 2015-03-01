#include "tcp.h"

////////////////////////////////////////////////////////////////////////////////

tTcpStatus tcp_toSockAddr(tTcpAddr tcp_addr, struct sockaddr_in* sock_addr)
{
    check_if(sock_addr == NULL, return TCP_ERROR, "sock_addr is null");

    sock_addr->sin_family = AF_INET;

    int check = inet_pton(AF_INET, tcp_addr.ipv4, &sock_addr->sin_addr);
    check_if(check != 1, return TCP_ERROR, "inet_pton failed");

    sock_addr->sin_port = htons(tcp_addr.port);

    return TCP_OK;
}

tTcpStatus tcp_toTcpAddr(struct sockaddr_in sock_addr, tTcpAddr* tcp_addr)
{
    check_if(tcp_addr == NULL, return TCP_ERROR, "tcp_addr is null");

    inet_ntop(AF_INET, &sock_addr.sin_addr, tcp_addr->ipv4, INET_ADDRSTRLEN);

    tcp_addr->port = ntohs(sock_addr.sin_port);

    return TCP_OK;
}

tTcpStatus tcp_server_init(tTcpServer* server, char* local_ip, int local_port, int max_conn_num)
{
    check_if(server == NULL, return TCP_ERROR, "server is null");
    check_if(max_conn_num <= 0, return TCP_ERROR, "max_conn_num = %d invalid", max_conn_num);

    int check;
    struct sockaddr_in me = {};
    me.sin_family = AF_INET;
    if (local_ip)
    {
        check = inet_pton(AF_INET, local_ip, &me.sin_addr);
        check_if(check != 1, return TCP_ERROR, "inet_pton failed");
    }
    else
    {
        me.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    me.sin_port = htons(local_port);

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    check_if(server->fd < 0, return TCP_ERROR, "sock failed");

    check = bind(server->fd, (struct sockaddr*)&me, sizeof(me));
    check_if(check < 0, goto _ERROR, "bind failed");

    check = listen(server->fd, max_conn_num);
    check_if(check < 0, goto _ERROR, "listen failed");

    server->local_port = local_port;
    server->is_init = 1;

    return TCP_OK;

_ERROR:
    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }
    return TCP_ERROR;
}

tTcpStatus tcp_server_uninit(tTcpServer* server)
{
    check_if(server == NULL, return TCP_ERROR, "server is null");
    check_if(server->is_init == 0, return TCP_ERROR, "server is not init yet");

    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }

    server->is_init = 0;
    return TCP_OK;
}

tTcpStatus tcp_server_accept(tTcpServer* server, tTcp* tcp)
{
    check_if(server == NULL, return TCP_ERROR, "server is null");
    check_if(server->is_init == 0, return TCP_ERROR, "server is not init yet");
    check_if(server->fd < 0, return TCP_ERROR, "server fd = %d invalid", server->fd);
    check_if(tcp == NULL, return TCP_ERROR, "tcp is null");

    int fd;
    struct sockaddr_in remote = {};
    int addrlen = sizeof(remote);

    fd = accept(server->fd, (struct sockaddr*)&remote, &addrlen);
    check_if(fd < 0, return TCP_ERROR, "accept failed");

    tTcpStatus ret = tcp_toTcpAddr(remote, &tcp->remote);
    check_if(ret != TCP_OK, goto _ERROR, "tcp_toTcpAddr failed");

    tcp->fd = fd;
    tcp->is_init = 1;

    return TCP_OK;

_ERROR:
    if (fd > 0)
    {
        close(fd);
    }
    return TCP_ERROR;
}

tTcpStatus tcp_client_init(tTcp* tcp, char* remote_ip, int remote_port, int local_port)
{
    check_if(tcp == NULL, return TCP_ERROR, "tcp is null");
    check_if(remote_ip == NULL, return TCP_ERROR, "remote_ip is null");
    check_if(tcp->is_init != 0, return TCP_ERROR, "tcp has been init");

    struct sockaddr_in remote = {};
    remote.sin_family = AF_INET;

    int check = inet_pton(AF_INET, remote_ip, &remote.sin_addr);
    check_if(check != 1, return TCP_ERROR, "inet_pton failed");

    remote.sin_port = htons(remote_port);

    tcp->fd = socket(AF_INET, SOCK_STREAM, 0);
    check_if(tcp->fd < 0, return TCP_ERROR, "socket failed");

    if (local_port != TCP_PORT_ANY)
    {
        struct sockaddr_in me = {};
        me.sin_family = AF_INET;
        me.sin_addr.s_addr = htonl(INADDR_ANY);
        me.sin_port = htons(local_port);

        const int on = 1;
        check = setsockopt(tcp->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
        check_if(check < 0, goto _ERROR, "setsockopt reuse failed");

        check = bind(tcp->fd, (struct sockaddr*)&me, sizeof(me));
        check_if(check < 0, goto _ERROR, "bind failed");
    }

    check = connect(tcp->fd, (struct sockaddr*)&remote, sizeof(remote));
    check_if(check < 0, goto _ERROR, "connect failed");

    int tcp_ret = tcp_toTcpAddr(remote, &tcp->remote);
    check_if(tcp_ret != TCP_OK, goto _ERROR, "tcp_toTcpAddr failed");

    tcp->is_init = 1;
    return TCP_OK;

_ERROR:
    if (tcp->fd > 0)
    {
        close(tcp->fd);
        tcp->fd = -1;
    }
    return TCP_ERROR;
}

tTcpStatus tcp_client_uninit(tTcp* tcp)
{
    check_if(tcp == NULL, return TCP_ERROR, "tcp is null");
    check_if(tcp->is_init == 0, return TCP_ERROR, "tcp is not init yet");

    if (tcp->fd > 0)
    {
        close(tcp->fd);
        tcp->fd = -1;
    }

    tcp->is_init = 0;

    return TCP_OK;
}

int tcp_recv(tTcp* tcp, void* buffer, int buffer_size)
{
    check_if(tcp == NULL, return -1, "tcp is null");
    check_if(tcp->is_init == 0, return -1, "tcp is not init yet");
    check_if(tcp->fd <= 0, return -1, "tcp fd <= 0");
    check_if(buffer == NULL, return -1, "buffer is null");
    check_if(buffer_size <= 0, return -1, "buffer_size = %d invalid", buffer_size);

    return recv(tcp->fd, buffer, buffer_size, 0);
}

int tcp_send(tTcp* tcp, void* data, int data_len)
{
    check_if(tcp == NULL, return -1, "tcp is null");
    check_if(tcp->is_init == 0, return -1, "tcp is not init yet");
    check_if(tcp->fd <= 0, return -1, "tcp fd <= 0");
    check_if(data == NULL, return -1, "data is null");
    check_if(data_len <= 0, return -1, "data_len = %d invalid", data_len);

    return send(tcp->fd, data, data_len, 0);
}

