#include "basic.h"
#include "tls.h"
#include "list.h"

#define ROOT_CA_PATH "../files/certifications/generated/rootA.pem"

#define SERVER_KEY_PATH "../files/certifications/generated/serverAkey.pem"
#define SERVER_CRT_PATH "../files/certifications/generated/serverAcert.pem"

#define CLIENT_KEY_PATH "../files/certifications/generated/clientAkey.pem"
#define CLIENT_CRT_PATH "../files/certifications/generated/clientAcert.pem"

static void clean_tls_peer(void* peer)
{
    if (peer)
    {
        struct tls* tls = (struct tls*)peer;
        tls_client_uninit(tls);
        free(tls);
    }
}

static int server_main(int local_port)
{
    int epfd = epoll_create(40);
    CHECK_IF(epfd <= 0, return -1, "epoll_create failed");

    struct tls_server server = {};
    int chk = tls_server_init(&server, NULL, local_port, 30, SERVER_CRT_PATH, SERVER_KEY_PATH, ROOT_CA_PATH);
    CHECK_IF(chk != TLS_OK, return -1, "tls_server_init failed");

    struct epoll_event ev = {};
    ev.data.fd = server.fd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server.fd, &ev);

    struct list peer_list;
    list_init(&peer_list, clean_tls_peer);

    while (1)
    {
        struct epoll_event evbuf[30] = {{0}};
        int ev_num = epoll_wait(epfd, evbuf, 30, -1);
        int i;
        for (i=0; i<ev_num; i++)
        {
            if (evbuf[i].events & EPOLLIN)
            {
                if (evbuf[i].data.fd == server.fd)
                {
                    struct tls* tls = calloc(sizeof(struct tls), 1);
                    chk = tls_server_accept(&server, tls);
                    if (chk == TLS_OK)
                    {
                        list_append(&peer_list, tls);

                        memset(&ev, 0, sizeof(struct epoll_event));
                        ev.data.fd = tls->fd;
                        ev.events = EPOLLIN;
                        epoll_ctl(epfd, EPOLL_CTL_ADD, tls->fd, &ev);
                    }
                    else
                    {
                        clean_tls_peer(tls);
                    }
                }
                else
                {
                    void* obj;
                    struct tls* peer;
                    struct tls* tls = NULL;
                    LIST_FOREACH(&peer_list, obj, peer)
                    {
                        if (evbuf[i].data.fd == peer->fd)
                        {
                            tls = peer;
                            break;
                        }
                    }

                    if (tls)
                    {
                        uint8_t buffer[256];
                        int recv_len = tls_recv(tls, buffer, 256);
                        if (recv_len > 0)
                        {
                            dprint("tls from (%s:%d) recv: %s", tls->remote.ipv4, tls->remote.port, buffer);
                            // const char* reply = "I Got your Message.";
                            // int send_len = tls_send(tls, (void*)reply, strlen(reply) + 1);
                            // if  (send_len > 0)
                            // {
                            //     dprint("tls send reply back to (%s:%d)", tls->remote.ipv4, tls->remote.port);
                            // }
                            // else
                            // {
                            //     int err = tls_get_sslerror(tls, recv_len);
                            //     dprint("tls from (%s:%d) send fail, error = %d", tls->remote.ipv4, tls->remote.port, err);

                            //     if (err == SSL_ERROR_ZERO_RETURN)
                            //     {
                            //         dprint("zero return: tls from (%s:%d) is shutdown", tls->remote.ipv4, tls->remote.port);
                            //         list_remove(&peer_list, tls);
                            //         clean_tls_peer(tls);
                            //     }
                            //     else if (err == SSL_ERROR_WANT_READ)
                            //     {
                            //         dprint("want read: tls from (%s:%d), do nothing", tls->remote.ipv4, tls->remote.port);
                            //     }
                            //     else if (err == SSL_ERROR_WANT_WRITE)
                            //     {
                            //         dprint("want write: tls from (%s:%d), do nothing", tls->remote.ipv4, tls->remote.port);
                            //     }
                            //     else
                            //     {
                            //         ERR_print_errors_fp(stderr);
                            //         dprint("unknown code %d: tls from (%s:%d), do nothing", err, tls->remote.ipv4, tls->remote.port);
                            //         list_remove(&peer_list, tls);
                            //         clean_tls_peer(tls);
                            //     }
                            // }
                        }
                        else
                        {
                            int err = tls_get_sslerror(tls, recv_len);
                            dprint("tls from (%s:%d) recv fail, error = %d", tls->remote.ipv4, tls->remote.port, err);

                            if (err == SSL_ERROR_ZERO_RETURN)
                            {
                                dprint("zero return: tls from (%s:%d) is shutdown", tls->remote.ipv4, tls->remote.port);
                                list_remove(&peer_list, tls);
                                clean_tls_peer(tls);
                            }
                            else if (err == SSL_ERROR_WANT_READ)
                            {
                                dprint("want read: tls from (%s:%d), do nothing", tls->remote.ipv4, tls->remote.port);
                            }
                            else if (err == SSL_ERROR_WANT_WRITE)
                            {
                                dprint("want write: tls from (%s:%d), do nothing", tls->remote.ipv4, tls->remote.port);
                            }
                            else
                            {
                                ERR_print_errors_fp(stderr);
                                dprint("unknown code %d: tls from (%s:%d), do nothing", err, tls->remote.ipv4, tls->remote.port);
                                list_remove(&peer_list, tls);
                                clean_tls_peer(tls);
                            }
                        }
                    }
                    else
                    {
                        derror("find no tls peer with fd = %d", evbuf[i].data.fd);
                    }
                }
            }
        }
    }

    list_clean(&peer_list);

    return 0;
}

static int client_main(char* remote_ip, int remote_port)
{
    struct tls tls[20] = {{0}};
    int i;
    for (i=0; i<20; i++)
    {
        int chk = tls_client_init(&tls[i], remote_ip, remote_port, TLS_PORT_ANY, CLIENT_CRT_PATH, CLIENT_KEY_PATH, ROOT_CA_PATH);
        CHECK_IF(chk != TLS_OK, return -1, "tls_client_init failed");
    }

    for (i=0; i<20; i++)
    {
        int j;
        for (j=0; j<10; j++)
        {
            char buffer[256];
            snprintf(buffer, 256, "I AM A TLS CLIENT %d, message %d", i, j);
            int send_len = tls_send(&tls[i], (void*)buffer, strlen(buffer) + 1);
            dprint("send_len = %d", send_len);
        }
    }

    while (1)
    {
        sleep(1);
    }


    // for (i=0; i<20; i++)
    // {
    //     tls_client_uninit(&tls[i]);
    // }

    // struct tls tls = {};
    // int chk = tls_client_init(&tls, remote_ip, remote_port, TLS_PORT_ANY, CLIENT_CRT_PATH, CLIENT_KEY_PATH, ROOT_CA_PATH);
    // CHECK_IF(chk != TLS_OK, return -1, "tls_client_init failed");

    // const char* request = "I AM A TLS CLIENT";
    // int send_len = tls_send(&tls, (void*)request, strlen(request) + 1);
    // dprint("send_len = %d", send_len);

    // char buffer[256];
    // int recv_len = tls_recv(&tls, buffer, 256);
    // if (recv_len > 0)
    // {
    //     dprint("recv reply: %s", buffer);
    // }
    // dprint("recv_len = %d", recv_len);

    // tls_client_uninit(&tls);

    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc <= 1)
    {
        derror("argument is invalid");
        return -1;
    }

    int ret = -1;

    tls_module_init();

    if (strcmp(argv[1], "-s") == 0)
    {
        // server mode
        int local_port = atoi(argv[2]);
        ret = server_main(local_port);
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        // client mode
        char* remote_ip = (char*)argv[2];
        int remote_port = atoi(argv[3]);
        ret = client_main(remote_ip, remote_port);
    }
    else
    {
        derror("unknown argv[1] = %s", argv[1]);
    }

    tls_module_uninit();

    return ret;
}
