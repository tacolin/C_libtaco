#include "dtls.h"

////////////////////////////////////////////////////////////////////////////////

#define DTLS_END 0xABCD

#define DTLS_CONN_TIMEOUT 1

#define DTLS_BUF_SIZE 65535

#define DTLS_COOKIE_SECRET_LEN 16

////////////////////////////////////////////////////////////////////////////////

static unsigned char    _cookie_secret[DTLS_COOKIE_SECRET_LEN] = {0};
static int              _cookie_initialized                    = 0;
static pthread_mutex_t* _mutex_buf                             = NULL;

////////////////////////////////////////////////////////////////////////////////

static void _showSocketError(void)
{
    switch (errno)
    {
        case EINTR:
            /* Interrupted system call.
             * Just ignore.
             */
            derror("Interrupted system call!");
            break;

        case EBADF:
            /* Invalid socket.
             * Must close connection.
             */
            derror("Invalid socket!");
            break;
#ifdef EHOSTDOWN
        case EHOSTDOWN:
            /* Host is down.
             * Just ignore, might be an attacker
             * sending fake ICMP messages.
             */
            derror("Host is down!");
            break;
#endif
#ifdef ECONNRESET
        case ECONNRESET:
            /* Connection reset by peer.
             * Just ignore, might be an attacker
             * sending fake ICMP messages.
             */
            derror("Connection reset by peer!");
            break;
#endif
        case ENOMEM:
            /* Out of memory.
             * Must close connection.
             */
            derror("Out of memory!");
            break;
        case EACCES:
            /* Permission denied.
             * Just ignore, we might be blocked
             * by some firewall policy. Try again
             * and hope for the best.
             */
            derror("Permission denied!");
            break;
        default:
            /* Something unexpected happened */
            derror("Unexpected error! (errno = %d)", errno);
            break;
    }

    return;
}

static int _verifyDtlsCallback(int ok, X509_STORE_CTX *ctx)
{
    /* This function should ask the user
     * if he trusts the received certificate.
     * Here we always trust.
     */
    return 1;
}

static unsigned long _dtlsIdCallback(void)
{
    return (unsigned long)pthread_self();
}

static void _sslLockFunc(int mode, int n, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
        pthread_mutex_lock(&_mutex_buf[n]);
    }
    else
    {
        pthread_mutex_unlock(&_mutex_buf[n]);
    }
}

static tDtlsStatus _checkSslWrite(SSL* ssl, char* buffer, int len)
{
    int ret = DTLS_ERROR;

    switch (SSL_get_error(ssl, len))
    {
        case SSL_ERROR_NONE:
            // dprint("wrote %d bytes", len);
            ret = DTLS_OK;
            break;

        case SSL_ERROR_WANT_WRITE:
            /* Just try again later */
            derror("SSL_ERROR_WANT_WRITE");
            break;

        case SSL_ERROR_WANT_READ:
            /* continue with reading */
            derror("SSL_ERROR_WANT_READ");
            break;

        case SSL_ERROR_SYSCALL:
            derror("Socket write error: ");
            _showSocketError();
            break;

        case SSL_ERROR_SSL:
            derror("SSL write error: ");
            derror("%s (%d)", ERR_error_string(ERR_get_error(), buffer),
                   SSL_get_error(ssl, len));
            break;

        default:
            derror("Unexpected error while writing!");
            break;
    }

    return ret;
}

static tDtlsStatus _checkSslRead(SSL* ssl, char* buffer, int len)
{
    int ret = DTLS_ERROR;

    switch (SSL_get_error(ssl, len))
    {
        case SSL_ERROR_NONE:
            // dprint("read %d bytes", (int) len);
            ret = DTLS_OK;
            break;

        case SSL_ERROR_ZERO_RETURN:
            // dprint("no data to read");
            ret = DTLS_END;
            break;

        case SSL_ERROR_WANT_READ:
            /* Stop reading on socket timeout, otherwise try again */
            if (BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP,
                        0, NULL))
            {
                derror("Timeout! No response received.");
            }
            break;

        case SSL_ERROR_SYSCALL:
            derror("Socket read error: ");
            _showSocketError();
            break;

        case SSL_ERROR_SSL:
            derror("SSL read error: ");
            derror("%s (%d)", ERR_error_string(ERR_get_error(), buffer),
                   SSL_get_error(ssl, len));
            break;

        default:
            derror("Unexpected error while reading!\n");
            break;
    }

    return ret;
}

static int _isDtlsAlive(SSL* ssl)
{
    return !(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN
             || SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN);
    // return !(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN);
}

static int _generateCookie(SSL *ssl, unsigned char *cookie,
                           unsigned int *cookie_len)
{
    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
    unsigned int length = 0, resultlength;
    union
    {
        struct sockaddr_storage ss;
        struct sockaddr_in      s4;
        struct sockaddr_in6     s6;
    } peer = {};

    /* Initialize a random secret */
    if (!_cookie_initialized)
    {
        if (!RAND_bytes(_cookie_secret, DTLS_COOKIE_SECRET_LEN))
        {
            derror("error setting random cookie secret");
            return 0;
        }
        _cookie_initialized = 1;
    }

    /* Read peer information */
    (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    /* Create buffer with peer's address and port */
    length = 0;
    switch (peer.ss.ss_family)
    {
        case AF_INET:
            length += sizeof(struct in_addr);
            break;

        case AF_INET6:
            length += sizeof(struct in6_addr);
            break;

        default:
            OPENSSL_assert(0);
            break;
    }

    length += sizeof(in_port_t);
    buffer = (unsigned char*) OPENSSL_malloc(length);

    if (buffer == NULL)
    {
        derror("out of memory\n");
        return 0;
    }

    switch (peer.ss.ss_family)
    {
        case AF_INET:
            memcpy(buffer,
                   &peer.s4.sin_port,
                   sizeof(in_port_t));
            memcpy(buffer + sizeof(peer.s4.sin_port),
                   &peer.s4.sin_addr,
                   sizeof(struct in_addr));
            break;

        case AF_INET6:
            memcpy(buffer,
                   &peer.s6.sin6_port,
                   sizeof(in_port_t));
            memcpy(buffer + sizeof(in_port_t),
                   &peer.s6.sin6_addr,
                   sizeof(struct in6_addr));
            break;

        default:
            OPENSSL_assert(0);
            break;
    }

    /* Calculate HMAC of buffer using the secret */
    HMAC(EVP_sha1(), (const void*) _cookie_secret, DTLS_COOKIE_SECRET_LEN,
         (const unsigned char*) buffer, length, result, &resultlength);

    OPENSSL_free(buffer);

    memcpy(cookie, result, resultlength);
    *cookie_len = resultlength;

    return 1;
}

static int _verifyCookie(SSL *ssl, unsigned char *cookie,
                         unsigned int cookie_len)
{
    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
    unsigned int length = 0, resultlength;
    union
    {
        struct sockaddr_storage ss;
        struct sockaddr_in      s4;
        struct sockaddr_in6     s6;
    } peer = {};

    /* If secret isn't initialized yet, the cookie can't be valid */
    if (!_cookie_initialized)
    {
        return 0;
    }

    /* Read peer information */
    (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    /* Create buffer with peer's address and port */
    length = 0;
    switch (peer.ss.ss_family)
    {
        case AF_INET:
            length += sizeof(struct in_addr);
            break;
        case AF_INET6:
            length += sizeof(struct in6_addr);
            break;
        default:
            OPENSSL_assert(0);
            break;
    }

    length += sizeof(in_port_t);
    buffer = (unsigned char*) OPENSSL_malloc(length);

    if (buffer == NULL)
    {
        derror("out of memory");
        return 0;
    }

    switch (peer.ss.ss_family)
    {
        case AF_INET:
            memcpy(buffer,
                   &peer.s4.sin_port,
                   sizeof(in_port_t));
            memcpy(buffer + sizeof(in_port_t),
                   &peer.s4.sin_addr,
                   sizeof(struct in_addr));
            break;

        case AF_INET6:
            memcpy(buffer,
                   &peer.s6.sin6_port,
                   sizeof(in_port_t));
            memcpy(buffer + sizeof(in_port_t),
                   &peer.s6.sin6_addr,
                   sizeof(struct in6_addr));
            break;

        default:
            OPENSSL_assert(0);
            break;
    }

    /* Calculate HMAC of buffer using the secret */
    HMAC(EVP_sha1(), (const void*) _cookie_secret, DTLS_COOKIE_SECRET_LEN,
         (const unsigned char*) buffer, length, result, &resultlength);

    OPENSSL_free(buffer);

    if (cookie_len == resultlength
        && memcmp(result, cookie, resultlength) == 0)
    {
        return 1;
    }

    return 0;
}



////////////////////////////////////////////////////////////////////////////////

tDtlsStatus dtls_toSockAddr(tDtlsAddr dtls_addr, struct sockaddr_in* sock_addr)
{
    check_if(sock_addr == NULL, return DTLS_ERROR, "sock_addr is null");

    sock_addr->sin_family = AF_INET;

    int check = inet_pton(AF_INET, dtls_addr.ipv4, &sock_addr->sin_addr);
    check_if(check != 1, return DTLS_ERROR, "inet_pton failed");

    sock_addr->sin_port = htons(dtls_addr.port);

    return DTLS_OK;
}

tDtlsStatus dtls_toDtlsAddr(struct sockaddr_in sock_addr, tDtlsAddr* dtls_addr)
{
    check_if(dtls_addr == NULL, return DTLS_ERROR, "dtls_addr is null");

    inet_ntop(AF_INET, &sock_addr.sin_addr, dtls_addr->ipv4, INET_ADDRSTRLEN);

    dtls_addr->port = ntohs(sock_addr.sin_port);

    return DTLS_OK;
}

////////////////////////////////////////////////////////////////////////////////

tDtlsStatus dtls_server_init(tDtlsServer* server,
                             char* local_ip, int local_port,
                             char* cert_path, char* key_path, int timeout_sec)
{
    check_if(server == NULL, return DTLS_ERROR, "server is null");
    check_if(cert_path == NULL, return DTLS_ERROR, "cert_path is null");
    check_if(key_path == NULL, return DTLS_ERROR, "key_path is null");
    check_if(timeout_sec <= 0, return DTLS_ERROR, "timeout_sec <= 0");

    memset(server, 0, sizeof(tDtlsServer));

    snprintf(server->cert_path, DTLS_PATH_SIZE, "%s", cert_path);
    snprintf(server->key_path, DTLS_PATH_SIZE, "%s", key_path);

    int check;
    struct sockaddr_in me = {};
    me.sin_family = AF_INET;
    if (local_ip)
    {
        check = inet_pton(AF_INET, local_ip, &me.sin_addr);
        check_if(check != 1, return DTLS_ERROR, "inet_pton failed");
    }
    else
    {
        me.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    me.sin_port = htons(local_port);

    tDtlsStatus dtls_ret = dtls_toDtlsAddr(me, &server->local);
    check_if(dtls_ret != DTLS_OK, return DTLS_ERROR, "dtls_toDtlsAddr failed");

    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();

    server->ctx = SSL_CTX_new(DTLSv1_server_method());

    // We accept all ciphers, including NULL.
    // Not recommended beyond testing and debugging

    SSL_CTX_set_cipher_list(server->ctx, "ALL:NULL:eNULL:aNULL");
    SSL_CTX_set_session_cache_mode(server->ctx, SSL_SESS_CACHE_OFF);

    if (!SSL_CTX_use_certificate_file(server->ctx, cert_path,
                                        SSL_FILETYPE_PEM))
    {
        derror("ERROR: no certificate found!");
        goto _ERROR;
    }

    if (!SSL_CTX_use_PrivateKey_file(server->ctx, key_path,
                                        SSL_FILETYPE_PEM))
    {
        derror("ERROR: no private key found!");
        goto _ERROR;
    }

    if (!SSL_CTX_check_private_key(server->ctx))
    {
        derror("ERROR: invalid private key!");
        goto _ERROR;
    }

    /* Client has to authenticate */
    SSL_CTX_set_verify(server->ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE,
                       _verifyDtlsCallback);
    SSL_CTX_set_read_ahead(server->ctx, 1);
    SSL_CTX_set_cookie_generate_cb(server->ctx, _generateCookie);
    SSL_CTX_set_cookie_verify_cb(server->ctx, _verifyCookie);

    const int on = 1, off = 0;
    server->fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_if(server->fd < 0, goto _ERROR, "socket create failed");

    check = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR,
                       (const void*)&on, (socklen_t)sizeof(on));
    check_if(check < 0, goto _ERROR, "setsockopt reuse addr failed");

    check = bind(server->fd, (const struct sockaddr*)&me, sizeof(struct sockaddr_in));
    check_if(check < 0, goto _ERROR, "bind AF_INET failed");

    server->is_init = 1;

    return DTLS_OK;

_ERROR:
    dtls_server_uninit(server);
    return DTLS_ERROR;
}

tDtlsStatus dtls_server_uninit(tDtlsServer* server)
{
    check_if(server == NULL, return DTLS_ERROR, "server is null");

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

    ERR_free_strings();
    EVP_cleanup();
    ERR_remove_state(0);

    server->is_init = 0;

    return DTLS_OK;
}

tDtlsStatus dtls_server_accept(tDtlsServer* server, tDtls* dtls)
{
    check_if(server == NULL, return DTLS_ERROR, "server is null");
    check_if(dtls == NULL, return DTLS_ERROR, "dtls is null");
    check_if(server->is_init != 1, return DTLS_ERROR, "server is not init yet");

    memset(dtls, 0, sizeof(tDtls));

    union {
        struct sockaddr_storage ss;
        struct sockaddr_in      s4;
        struct sockaddr_in6     s6;
    } remote;

    /* Create BIO */
    dtls->bio = BIO_new_dgram(server->fd, BIO_NOCLOSE);
    check_if(dtls->bio == NULL, goto _ERROR, "BIO_new_dgram failed");

    /* Set and activate timeouts */
    struct timeval timeout = {};
    timeout.tv_sec  = DTLS_CONN_TIMEOUT;
    timeout.tv_usec = 0;

    BIO_ctrl(dtls->bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    dtls->ssl = SSL_new(server->ctx);
    check_if(dtls->ssl == NULL, goto _ERROR, "SSL_new failed");

    SSL_set_bio(dtls->ssl, dtls->bio, dtls->bio);
    SSL_set_options(dtls->ssl, SSL_OP_COOKIE_EXCHANGE);

    server->accept_run = 1;

    while (DTLSv1_listen(dtls->ssl, &remote) <= 0)
    {
        if (server->accept_run != 1) goto _ERROR;
    }

    tDtlsStatus dtls_ret = dtls_toDtlsAddr(remote.s4, &dtls->remote);
    check_if(dtls_ret != DTLS_OK, goto _ERROR, "dtls_toDtlsAddr failed");

    struct sockaddr_in me = {};
    dtls_ret = dtls_toSockAddr(server->local, &me);
    check_if(dtls_ret != DTLS_OK, goto _ERROR, "dtls_toSockAddr failed");

    dtls->fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_if(dtls->fd < 0, goto _ERROR, "socket failed");

    const int on = 1, off = 0;
    int check = setsockopt(dtls->fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&on,
                       (socklen_t)sizeof(on));
    check_if(check < 0, goto _ERROR, "setsockopt reuse addr failed");

    check = bind(dtls->fd, (const struct sockaddr*)&me, sizeof(struct sockaddr_in));
    check_if(check < 0, goto _ERROR, "bind failed");

    check = connect(dtls->fd, (struct sockaddr*)&remote.s4, sizeof(struct sockaddr_in));
    check_if(check < 0, goto _ERROR, "connect failed");

    struct sockaddr_storage ss = {.ss_family = AF_INET};

    /* Set new fd and set BIO to connected */
    BIO_set_fd(SSL_get_rbio(dtls->ssl), dtls->fd, BIO_NOCLOSE);
    BIO_ctrl(SSL_get_rbio(dtls->ssl), BIO_CTRL_DGRAM_SET_CONNECTED, 0, &ss);

    /* Finish handshake */
    do
    {
        check = SSL_accept(dtls->ssl);

    } while (check == 0);

    if (check < 0)
    {
        char buffer[DTLS_BUF_SIZE];
        derror("SSL_accept");
        derror("%s", ERR_error_string(ERR_get_error(), buffer));
        goto _ERROR;
    }

    timeout.tv_sec = server->timeout_sec;
    timeout.tv_usec = 0;

    BIO_ctrl(SSL_get_rbio(dtls->ssl), BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    X509* pX509 = SSL_get_peer_certificate(dtls->ssl);
    if (pX509)
    {
        printf("\n");
        X509_NAME_print_ex_fp(stdout, X509_get_subject_name(pX509),
                              1, XN_FLAG_MULTILINE);
        printf("\n\n");
        // printf("Cipher: %s",
        //        SSL_CIPHER_get_name(SSL_get_current_cipher(info->ssl)));
        // printf("-----------------------------------------------------------\n");

        X509_free(pX509);
    }

    dtls->is_init = 1;

    return DTLS_OK;

_ERROR:
    dtls_client_uninit(dtls);
    return DTLS_ERROR;

}

////////////////////////////////////////////////////////////////////////////////

tDtlsStatus dtls_client_init(tDtls* dtls, char* remote_ip, int remote_port,
                             int local_port, char* cert_path, char* key_path,
                             int timeout_sec)
{
    check_if(dtls == NULL, return DTLS_ERROR, "dtls is null");
    check_if(remote_ip == NULL, return DTLS_ERROR, "remote_ip is null");
    check_if(cert_path == NULL, return DTLS_ERROR, "cert_path is null");
    check_if(key_path == NULL, return DTLS_ERROR, "key_path is null");
    check_if(timeout_sec <= 0, return DTLS_ERROR, "timeout_sec = %d invalid", timeout_sec);

    memset(dtls, 0, sizeof(tDtls));

    snprintf(dtls->cert_path, DTLS_PATH_SIZE, "%s", cert_path);
    snprintf(dtls->key_path, DTLS_PATH_SIZE, "%s", key_path);

    struct timeval timeout = {};
    timeout.tv_sec  = timeout_sec;
    timeout.tv_usec = 0;

    dtls->fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_if(dtls->fd < 0, return DTLS_ERROR, "socket failed");

    struct sockaddr_in me = {};
    me.sin_family = AF_INET;
    if (local_port != DTLS_PORT_ANY)
    {
        me.sin_port = htons(local_port);
    }

    struct sockaddr_in remote = {};
    remote.sin_family = AF_INET;
    int check = inet_pton(AF_INET, remote_ip, &remote.sin_addr);
    check_if(check != 1, goto _ERROR, "inet_pton failed");
    remote.sin_port = htons(remote_port);

    tDtlsStatus dtls_ret = dtls_toDtlsAddr(remote, &dtls->remote);
    check_if(dtls_ret != DTLS_OK, goto _ERROR, "dtls_toDtlsAddr failed");

    const int on = 1;
    check = setsockopt(dtls->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    check_if(check < 0, goto _ERROR, "setsockopt reuse addr failed");

    check = bind(dtls->fd, (struct sockaddr*)&me, sizeof(struct sockaddr_in));
    check_if(check < 0, goto _ERROR, "bind failed");

    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();

    dtls->ctx = SSL_CTX_new(DTLSv1_client_method());
    SSL_CTX_set_cipher_list(dtls->ctx, "eNULL:!MD5");

    if (!SSL_CTX_use_certificate_file(dtls->ctx, dtls->cert_path,
                                        SSL_FILETYPE_PEM))
    {
        derror("ERROR: no certificate found!");
        goto _ERROR;
    }

    if (!SSL_CTX_use_PrivateKey_file(dtls->ctx, dtls->key_path,
                                        SSL_FILETYPE_PEM))
    {
        derror("ERROR: no private key found!");
        goto _ERROR;
    }

    if (!SSL_CTX_check_private_key(dtls->ctx))
    {
        derror("ERROR: invalid private key!");
        goto _ERROR;
    }

    SSL_CTX_set_verify_depth(dtls->ctx, 2);
    SSL_CTX_set_read_ahead(dtls->ctx, 1);

    dtls->ssl = SSL_new(dtls->ctx);

    /* Create BIO, connect and set to already connected */
    dtls->bio = BIO_new_dgram(dtls->fd, BIO_CLOSE);

    check = connect(dtls->fd, (struct sockaddr*)&remote, sizeof(struct sockaddr_in));
    check_if(check < 0, goto _ERROR, "connect failed");

    struct sockaddr_storage ss = {.ss_family = AF_INET};

    BIO_ctrl(dtls->bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &ss);

    SSL_set_bio(dtls->ssl, dtls->bio, dtls->bio);

    check = SSL_connect(dtls->ssl);
    check_if(check < 0, goto _ERROR, "SSL_connect failed");

    BIO_ctrl(dtls->bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    //////////////////////////////////////////////////////////////////
    // SSL_get_peer_certificate will allocate a new memory for X509
    // do remember to free it
    //////////////////////////////////////////////////////////////////
    X509* pX509 = SSL_get_peer_certificate(dtls->ssl);
    if (pX509)
    {
        printf("\n");
        X509_NAME_print_ex_fp(stdout, X509_get_subject_name(pX509),
                              1, XN_FLAG_MULTILINE);
        printf("\n\n");

        X509_free(pX509);
    }

    dtls->is_init = 1;

    return DTLS_OK;

_ERROR:
    dtls_client_uninit(dtls);
    return DTLS_ERROR;
}

tDtlsStatus dtls_client_uninit(tDtls* dtls)
{
    check_if(dtls == NULL, return DTLS_ERROR, "dtls is null");
    // check_if(dtls->is_init != 1, return DTLS_ERROR, "dtls is not init yet");

    if (dtls->ssl)
    {
        SSL_shutdown(dtls->ssl);
        SSL_free(dtls->ssl);
        dtls->ssl = NULL;
    }

    if (dtls->fd > 0)
    {
        close(dtls->fd);
        dtls->fd = -1;
    }

    if (dtls->ctx)
    {
        SSL_CTX_free(dtls->ctx);
        dtls->ctx = NULL;
    }

    ERR_remove_state(0);
    ERR_free_strings();
    EVP_cleanup();

    dtls->is_init = 0;

    return DTLS_OK;
}

////////////////////////////////////////////////////////////////////////////////

int dtls_recv(tDtls* dtls, void* buffer, int buffer_size)
{
    check_if(dtls == NULL, return -1, "dtls is null");
    check_if(buffer == NULL, return -1, "buffer is null");
    check_if(buffer_size <= 0, return -1, "buffer_size <= 0");
    check_if(dtls->is_init != 1, return -1, "dtls is not init yet");
    check_if(!_isDtlsAlive(dtls->ssl), return -1, "dtls ssl is not alive");

    int readlen = SSL_read(dtls->ssl, buffer, buffer_size);
    int check = _checkSslRead(dtls->ssl, buffer, readlen);
    check_if(check != DTLS_OK, return -1, "_checkSslRead failed");

    return readlen;
}

int dtls_send(tDtls* dtls, void* data, int data_len)
{
    check_if(dtls == NULL, return -1, "dtls is null");
    check_if(data == NULL, return -1, "data is null");
    check_if(data_len <= 0, return -1, "data_len <= 0");
    check_if(dtls->is_init != 1, return -1, "dtls is not init yet");
    check_if(!_isDtlsAlive(dtls->ssl), return -1, "dtls ssl is not alive");

    int writelen = SSL_write(dtls->ssl, data, data_len);
    int check = _checkSslWrite(dtls->ssl, data, writelen);
    check_if(check != DTLS_OK, return -1, "_checkSslWrite failed");

    return writelen;
}

////////////////////////////////////////////////////////////////////////////////

tDtlsStatus dtls_system_init(void)
{
    _mutex_buf = (pthread_mutex_t*)calloc(sizeof(pthread_mutex_t),
                                          CRYPTO_num_locks());
    check_if(_mutex_buf == NULL, return DTLS_ERROR, "calloc failed");

    int i;
    for (i=0; i<CRYPTO_num_locks(); i++)
    {
        pthread_mutex_init(&_mutex_buf[i], NULL);
    }

    CRYPTO_set_id_callback(_dtlsIdCallback);
    CRYPTO_set_locking_callback(_sslLockFunc);

    return DTLS_OK;
}

void dtls_system_uninit(void)
{
    check_if(_mutex_buf == NULL, return, "_mutex_buf is null");

    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    int i;
    for (i=0; i<CRYPTO_num_locks(); i++)
    {
        pthread_mutex_destroy(&_mutex_buf[i]);
    }

    free(_mutex_buf);
    _mutex_buf = NULL;

    return;
}

