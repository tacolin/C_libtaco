#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tls.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}


static void set_ctx_auto_retry(SSL_CTX* ctx)
{
    long mode = SSL_CTX_get_mode(ctx);
    mode |= SSL_MODE_AUTO_RETRY;
    SSL_CTX_set_mode(ctx, mode);
    return;
}

static void set_ssl_auto_retry(SSL* ssl)
{
    long mode = SSL_get_mode(ssl);
    mode |= SSL_MODE_AUTO_RETRY;
    SSL_set_mode(ssl, mode);
}

static int set_socket_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    CHECK_IF(flags == -1, return -1, "F_GETFL failed");

    // Non-blocking
    flags |= O_NONBLOCK;
    int s = fcntl(fd, F_SETFL, flags);
    CHECK_IF(s == -1, return -1, "F_SETFL failed");
    return 0;
}

static int set_socket_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    CHECK_IF(flags == -1, return -1, "F_GETFL failed");

    // blocking
    flags &= ~O_NONBLOCK;
    int s = fcntl(fd, F_SETFL, flags);
    CHECK_IF(s == -1, return -1, "F_SETFL failed");
    return 0;
}

static int set_socket_keepalive(int fd)
{
    int value = 1;
    socklen_t len = sizeof(value);
    int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &value, len);
    CHECK_IF(ret != 0, return -1, "setsocketopt SO_KEEPALIVE failed");

    value = 20;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &value, len);
    CHECK_IF(ret != 0, return -1, "setsockopt TCP_KEEPIDLE value failed");

    value = 2;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &value, len);
    CHECK_IF(ret != 0, return -1, "setsockopt TCP_KEEPINTVL value failed");

    value = 3;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &value, len);
    CHECK_IF(ret != 0, return -1, "setsockopt TCP_KEEPCNT value failed");

    return 0;
}

static SSL_CTX* create_ssl_ctx(char *crt, char *key, char* rootca)
{
    const SSL_METHOD* meth = SSLv23_method();
    SSL_CTX* ctx = SSL_CTX_new( meth );
    if (!ctx)
    {
        derror("SSL_CTX_new failed");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (SSL_CTX_use_certificate_file(ctx, crt, SSL_FILETYPE_PEM) <= 0)
    {
        derror("SSL_CTX_use_certificate_file failed");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0)
    {
        derror("SSL_CTX_use_PrivateKey_file failed");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (!SSL_CTX_check_private_key(ctx))
    {
        derror("SSL_CTX_check_private_key failed");
        derror("Private key does not match the certificate public key.");
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Enable CA verification
    if (1)
    {
        /* Load the RSA CA certificate into the SSL_CTX structure */
        if (!SSL_CTX_load_verify_locations(ctx, rootca, NULL))
        {
            derror("SSL_CTX_load_verify_locations failed");
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(ctx);
            return NULL;
        }
        /* Set to require peer (client) certificate verification */
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
        /* Set the verification depth to 1 */
        SSL_CTX_set_verify_depth(ctx, 1);
    }

    set_ctx_auto_retry(ctx);
    return ctx;
}

static int retry_client_handshake(int new_fd, SSL* ssl)
{
    int epfd = epoll_create(3);
    CHECK_IF(epfd <= 0, return TLS_FAIL, "epoll_create failed");

    struct epoll_event ev = {};
    ev.data.fd = new_fd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &ev);

    int h_num;
    int ret;
    for (h_num=0; h_num<50; h_num++)
    {
        struct epoll_event events[3] = {{0}};
        int ev_num = epoll_wait(epfd, events, 3, 100);
        if (ev_num == 0)
        {
            // Timeout
            derror("dohandshake retry timeout");
            goto _ERROR;
        }

        int i;
        for (i=0; i<ev_num; i++)
        {
            if (events[i].events & EPOLLIN || events[i].events & EPOLLOUT)
            {
                ret = SSL_connect(ssl);
            }
        }

        if (ret > 0)
        {
            // SSL_connect success
            break;
        }

        switch (SSL_get_error(ssl, ret))
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                break;
            default:
                derror("SSL_get_error() = %d", SSL_get_error(ssl, ret));
                goto _ERROR;
        }
    }
    epoll_ctl(epfd, EPOLL_CTL_DEL, new_fd, &ev);
    close(epfd);
    return TLS_OK;

_ERROR:
    if (epfd > 0)
    {
        epoll_ctl(epfd, EPOLL_CTL_DEL, new_fd, &ev);
        close(epfd);
    }
    derror("ret final value = %d, tls accept failed", ret);
    ERR_print_errors_fp(stderr);
    return TLS_FAIL;
}

static int retry_server_handshake(int new_fd, SSL* ssl)
{
    int epfd = epoll_create(3);
    CHECK_IF(epfd <= 0, return TLS_FAIL, "epoll_create failed");

    struct epoll_event ev = {};
    ev.data.fd = new_fd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &ev);

    int h_num;
    int ret;
    for (h_num=0; h_num<50; h_num++)
    {
        struct epoll_event events[3] = {{0}};
        int ev_num = epoll_wait(epfd, events, 3, 100);
        if (ev_num == 0)
        {
            // Timeout
            derror("dohandshake retry timeout");
            goto _ERROR;
        }

        int i;
        for (i=0; i<ev_num; i++)
        {
            if (events[i].events & EPOLLIN || events[i].events & EPOLLOUT)
            {
                ret = SSL_accept(ssl);
            }
        }

        if (ret > 0)
        {
            // SSL_accept success
            break;
        }

        switch (SSL_get_error(ssl, ret))
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                break;
            default:
                derror("SSL_get_error() = %d", SSL_get_error(ssl, ret));
                goto _ERROR;
        }
    }
    epoll_ctl(epfd, EPOLL_CTL_DEL, new_fd, &ev);
    close(epfd);
    return TLS_OK;

_ERROR:
    if (epfd > 0)
    {
        epoll_ctl(epfd, EPOLL_CTL_DEL, new_fd, &ev);
        close(epfd);
    }
    derror("ret final value = %d, tls accept failed", ret);
    ERR_print_errors_fp(stderr);
    return TLS_FAIL;
}

int tls_server_init(struct tls_server* server, char* local_ip, int local_port, int max_conn_num, char* crt, char* key, char* rootca)
{
    CHECK_IF(server == NULL, return TLS_FAIL, "server is null");
    CHECK_IF(max_conn_num <= 0, return TLS_FAIL, "max_conn_num = %d invalid", max_conn_num);
    CHECK_IF(local_port <= 0, return TLS_FAIL, "local_port = %d invalid", local_port);
    CHECK_IF(crt == NULL, return TLS_FAIL, "crt is null");
    CHECK_IF(key == NULL, return TLS_FAIL, "key is null");
    CHECK_IF(rootca == NULL, return TLS_FAIL, "rootca is null");

    int chk;
    SSL_CTX* ctx = NULL;
    struct sockaddr_in me = {};
    me.sin_family = AF_INET;
    if (local_ip)
    {
        chk = inet_pton(AF_INET, local_ip, &me.sin_addr);
        CHECK_IF(chk != 1, return TLS_FAIL, "inet_pton failed");
    }
    else
    {
        me.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    me.sin_port = htons(local_port);

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_IF(server->fd < 0, return TLS_FAIL, "sock failed");

    int on = 1;
    chk = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuseaddr failed");

    chk = bind(server->fd, (struct sockaddr*)&me, sizeof(me));
    CHECK_IF(chk < 0, goto _ERROR, "bind failed");

    chk = listen(server->fd, max_conn_num);
    CHECK_IF(chk < 0, goto _ERROR, "listen failed");

    server->local_port = local_port;

    ctx = create_ssl_ctx(crt, key, rootca);
    CHECK_IF(ctx == NULL, goto _ERROR, "create_ssl_ctx failed");

    server->ctx = ctx;
    server->is_init    = 1;
    return TLS_OK;

_ERROR:
    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }
    if (ctx)
    {
        SSL_CTX_free(ctx);
    }
    return TLS_FAIL;
}

int tls_server_uninit(struct tls_server* server)
{
    CHECK_IF(server == NULL, return TLS_FAIL, "server is null");
    CHECK_IF(server->is_init == 0, return TLS_FAIL, "server is not init yet");

    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }

    if (server->ctx)
    {
        SSL_CTX_free(server->ctx);
        server->ctx = NULL;
    }

    server->is_init = 0;
    return TLS_OK;
}

int tls_server_accept(struct tls_server* server, struct tls* tls)
{
    CHECK_IF(server == NULL, return TLS_FAIL, "server is null");
    CHECK_IF(server->is_init == 0, return TLS_FAIL, "server is not init yet");
    CHECK_IF(server->fd < 0, return TLS_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(tls == NULL, return TLS_FAIL, "tls is null");

    struct sockaddr_in remote = {};
    int addrlen = sizeof(remote);

    int fd = accept(server->fd, (struct sockaddr*)&remote, &addrlen);
    CHECK_IF(fd < 0, return TLS_FAIL, "accept failed");

    int chk = tls_to_tlsaddr(remote, &tls->remote);
    CHECK_IF(chk != TLS_OK, goto _ERROR, "tls_to_tlsaddr failed");

    SSL* ssl = SSL_new(server->ctx);
    CHECK_IF(ssl == NULL, goto _ERROR, "SSL_new failed");
    SSL_set_fd(ssl, fd);

    set_socket_non_blocking(fd);

    chk = retry_server_handshake(fd, ssl);
    CHECK_IF(chk != TLS_OK, goto _ERROR, "retry_server_handshake failed with %s:%d", tls->remote.ipv4, tls->remote.port);

    set_socket_blocking(fd);

    set_ssl_auto_retry(ssl);
    set_socket_keepalive(fd);

    tls->fd = fd;
    tls->ssl = ssl;
    tls->is_init = 1;

    return TLS_OK;

_ERROR:
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (fd > 0)
    {
        close(fd);
    }
    return TLS_FAIL;
}

int tls_client_init(struct tls* tls, char* remote_ip, int remote_port, int local_port, char* crt, char* key, char* rootca)
{
    CHECK_IF(tls == NULL, return TLS_FAIL, "tls is null");
    CHECK_IF(remote_ip == NULL, return TLS_FAIL, "remote_ip is null");
    CHECK_IF(tls->is_init != 0, return TLS_FAIL, "tls has been init");

    SSL_CTX* ctx = NULL;
    SSL* ssl = NULL;

    struct sockaddr_in remote = {};
    remote.sin_family = AF_INET;

    int chk = inet_pton(AF_INET, remote_ip, &remote.sin_addr);
    CHECK_IF(chk != 1, return TLS_FAIL, "inet_pton failed");

    remote.sin_port = htons(remote_port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_IF(fd < 0, return TLS_FAIL, "socket failed");

    if (local_port != TLS_PORT_ANY)
    {
        struct sockaddr_in me = {};
        me.sin_family = AF_INET;
        me.sin_addr.s_addr = htonl(INADDR_ANY);
        me.sin_port = htons(local_port);

        const int on = 1;
        chk = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
        CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuse failed");

        chk = bind(fd, (struct sockaddr*)&me, sizeof(me));
        CHECK_IF(chk < 0, goto _ERROR, "bind failed");
    }

    chk = connect(fd, (struct sockaddr*)&remote, sizeof(remote));
    CHECK_IF(chk < 0, goto _ERROR, "connect failed");

    chk = tls_to_tlsaddr(remote, &tls->remote);
    CHECK_IF(chk != TLS_OK, goto _ERROR, "tls_to_tlsaddr failed");

    ctx = create_ssl_ctx(crt, key, rootca);
    CHECK_IF(!ctx, goto _ERROR, "create_ssl_ctx failed");

    ssl = SSL_new(ctx);
    CHECK_IF(!ssl, goto _ERROR, "SSL_new failed");

    SSL_set_fd(ssl, fd);

    set_socket_non_blocking(fd);

    chk = retry_client_handshake(fd, ssl);
    CHECK_IF(chk != TLS_OK, goto _ERROR, "retry_client_handshake failed");

    set_socket_blocking(fd);

    set_ssl_auto_retry(ssl);
    set_socket_keepalive(fd);

    tls->fd = fd;
    tls->ctx = ctx;
    tls->ssl = ssl;
    tls->is_init = 1;
    return TLS_OK;

_ERROR:
    if (ctx)
    {
        SSL_CTX_free(ctx);
    }
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (fd > 0)
    {
        close(fd);
    }
    return TLS_FAIL;
}

int tls_client_uninit(struct tls* tls)
{
    CHECK_IF(tls == NULL, return TLS_FAIL, "tls is null");
    CHECK_IF(tls->is_init == 0, return TLS_FAIL, "tls is not init yet");

    if (tls->ssl)
    {
        SSL_shutdown(tls->ssl);
        SSL_free(tls->ssl);
    }
    tls->ssl = NULL;

    if (tls->ctx)
    {
        SSL_CTX_free(tls->ctx);
    }
    tls->ctx = NULL;

    if (tls->fd > 0)
    {
        close(tls->fd);
        tls->fd = -1;
    }

    tls->is_init = 0;
    return TLS_OK;
}

int tls_recv(struct tls* tls, void* buffer, int buffer_size)
{
    CHECK_IF(tls == NULL, return -1, "tls is null");
    CHECK_IF(tls->is_init == 0, return -1, "tls is not init yet");
    CHECK_IF(tls->fd <= 0, return -1, "tls fd <= 0");
    CHECK_IF(buffer == NULL, return -1, "buffer is null");
    CHECK_IF(buffer_size <= 0, return -1, "buffer_size = %d invalid", buffer_size);
    return SSL_read(tls->ssl, buffer, buffer_size);
}

int tls_send(struct tls* tls, void* data, int data_len)
{
    CHECK_IF(tls == NULL, return -1, "tls is null");
    CHECK_IF(tls->is_init == 0, return -1, "tls is not init yet");
    CHECK_IF(tls->fd <= 0, return -1, "tls fd <= 0");
    CHECK_IF(data == NULL, return -1, "data is null");
    CHECK_IF(data_len <= 0, return -1, "data_len = %d invalid", data_len);
    return SSL_write(tls->ssl, data, data_len);
}

int tls_to_sockaddr(struct tls_addr tls_addr, struct sockaddr_in* sock_addr)
{
    CHECK_IF(sock_addr == NULL, return TLS_FAIL, "sock_addr is null");

    int chk = inet_pton(AF_INET, tls_addr.ipv4, &sock_addr->sin_addr);
    CHECK_IF(chk != 1, return TLS_FAIL, "inet_pton failed");

    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port   = htons(tls_addr.port);
    return TLS_OK;
}

int tls_to_tlsaddr(struct sockaddr_in sock_addr, struct tls_addr* tls_addr)
{
    CHECK_IF(tls_addr == NULL, return TLS_FAIL, "tls_addr is null");

    inet_ntop(AF_INET, &sock_addr.sin_addr, tls_addr->ipv4, INET_ADDRSTRLEN);
    tls_addr->port = ntohs(sock_addr.sin_port);
    return TLS_OK;
}

int tls_get_sslerror(struct tls* tls, int return_value)
{
    CHECK_IF(tls == NULL, return SSL_ERROR_SSL, "tls is null");
    return SSL_get_error(tls->ssl, return_value);
}

void tls_module_init(void)
{
    SSL_library_init();
    SSL_load_error_strings();
}

void tls_module_uninit(void)
{
    ERR_free_strings();
    EVP_cleanup();
    ERR_remove_state(0);
}
