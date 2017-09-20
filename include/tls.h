#ifndef _TLS_H_
#define _TLS_H_

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/epoll.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define TLS_PORT_ANY -1
#define TLS_OK (0)
#define TLS_FAIL (-1)

// if you send more the 16384 bytes in tls_send(), your packet will be fragmenet by ip layer.
#define TLS_MAGIC_NUMBER (16384)

struct tls_addr
{
    char ipv4[INET_ADDRSTRLEN];
    int  port;
};

struct tls
{
    int fd;
    int local_port;
    int is_init;
    struct tls_addr remote;

    SSL_CTX* ctx;
    SSL* ssl;
};

struct tls_server
{
    int fd;
    int local_port;
    int is_init;

    SSL_CTX* ctx;
};

int tls_server_init(struct tls_server* server, char* local_ip, int local_port, int max_conn_num, char* crt, char* key, char* rootca);
int tls_server_uninit(struct tls_server* server);
int tls_server_accept(struct tls_server* server, struct tls* tls);

int tls_client_init(struct tls* tls, char* remote_ip, int remote_port, int local_port, char* crt, char* key, char* rootca);
int tls_client_uninit(struct tls* tls);

int tls_recv(struct tls* tls, void* buffer, int buffer_size);
int tls_send(struct tls* tls, void* data, int data_len);

int tls_to_sockaddr(struct tls_addr tls_addr, struct sockaddr_in* sock_addr);
int tls_to_tlsaddr(struct sockaddr_in sock_addr, struct tls_addr* tls_addr);

int tls_get_sslerror(struct tls* tls, int return_value);

void tls_module_init(void);
void tls_module_uninit(void);

#endif //_TLS_H_
