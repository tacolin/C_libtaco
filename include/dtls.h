#ifndef _DTLS_H_
#define _DTLS_H_

#include "basic.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

////////////////////////////////////////////////////////////////////////////////

#define DTLS_PATH_SIZE 108
#define DTLS_PORT_ANY -1

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    DTLS_OK = 0,
    DTLS_ERROR = -1,

} tDtlsStatus;

typedef struct tDtlsAddr
{
    char ipv4[INET_ADDRSTRLEN];
    int port;

} tDtlsAddr;

typedef struct tDtlsServer
{
    int fd;
    int local_port;
    int timeout_sec;
    char cert_path[DTLS_PATH_SIZE];
    char key_path[DTLS_PATH_SIZE];

    tDtlsAddr local;

    SSL_CTX* ctx;

    int is_init;

    int accept_run;

} tDtlsServer;

typedef struct tDtls
{
    int fd;
    int local_port;
    int timeout_sec;
    char cert_path[DTLS_PATH_SIZE];
    char key_path[DTLS_PATH_SIZE];

    tDtlsAddr remote;

    SSL_CTX* ctx;
    SSL* ssl;
    BIO* bio;

    int is_init;

} tDtls;

////////////////////////////////////////////////////////////////////////////////

tDtlsStatus dtls_toSockAddr(tDtlsAddr dtls_addr, struct sockaddr_in* sock_addr);

tDtlsStatus dtls_toDtlsAddr(struct sockaddr_in sock_addr, tDtlsAddr* dtls_addr);

tDtlsStatus dtls_server_init(tDtlsServer* server,
                             char* local_ip, int local_port,
                             char* cert_path, char* key_path, int timeout_sec);
tDtlsStatus dtls_server_uninit(tDtlsServer* server);
tDtlsStatus dtls_server_accept(tDtlsServer* server, tDtls* dtls);

tDtlsStatus dtls_client_init(tDtls* dtls, char* remote_ip, int remote_port,
                             int local_port, char* cert_path, char* key_path,
                             int timeout_sec);
tDtlsStatus dtls_client_uninit(tDtls* dtls);

int dtls_recv(tDtls* dtls, void* buffer, int buffer_size);
int dtls_send(tDtls* dtls, void* data, int data_len);

// tDtlsStatus dtls_system_init(void);
// void dtls_system_uninit(void);

#endif //_DTLS_H_
