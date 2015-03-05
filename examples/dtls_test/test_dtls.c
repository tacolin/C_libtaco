#include "basic.h"
#include "dtls.h"
#include "list.h"

#include <signal.h>
#include <string.h>
#include <sys/select.h>

static int _running = 1;
static tDtlsServer _server = {};

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    _server.accept_run = 0;
    dprint("stop main()");
}

static void _destroyDtls(void* arg)
{
    check_if(arg == NULL, return, "arg is null");
    tDtls* dtls = (tDtls*)arg;
    dtls_client_uninit(dtls);
    free(dtls);
    return;
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    if (argc == 2)
    {
        //////////////////////////////////////////////
        //  Server Mode
        //  ./test_dtls [local port]
        //
        //  for example:
        //  ./test_dtls 50000
        //////////////////////////////////////////////

        int port = atoi(argv[1]);

        dtls_server_init(&_server, NULL, port,
                         "certs/server-cert.pem", "certs/server-key.pem", 10);

        char buffer[256];
        int  recvlen;
        int  sendlen;

        // while (_running)
        // {
        //     tDtls dtls = {};
        //     dtls_server_accept(&_server, &dtls);

        //     while (_running)
        //     {
        //         recvlen = dtls_recv(&dtls, buffer, 256);
        //         if (recvlen <= 0)
        //         {
        //             derror("recvlen = %d", recvlen);
        //             break;
        //         }

        //         dprint("rx (%s) from ip(%s) port(%d)", buffer,
        //             dtls.remote.ipv4, dtls.remote.port);

        //         sendlen = dtls_send(&dtls, buffer, recvlen);
        //         if (sendlen <= 0)
        //         {
        //             derror("send echo back len = %d", sendlen);
        //             break;
        //         }
        //     }

        //     dtls_client_uninit(&dtls);
        // }

        // dtls_server_uninit(&_server);


        struct timeval timeout = {};
        int sel_ret;
        fd_set readset;
        tList dtls_list;
        tDtls* dtls = NULL;

        list_init(&dtls_list, _destroyDtls);

        while (_running)
        {
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            FD_ZERO(&readset);

            FD_SET(_server.fd, &readset);

            for (dtls = list_head(&dtls_list); dtls; dtls = list_next(&dtls_list, dtls))
            {
                FD_SET(dtls->fd, &readset);
            }

            sel_ret = select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
            if (sel_ret < 0)
            {
                derror("select failed");
                break;
            }
            else if (sel_ret == 0)
            {
                continue;
            }
            else
            {
                if (FD_ISSET(_server.fd, &readset))
                {
                    dtls = calloc(sizeof(tDtls), 1);
                    dtls_server_accept(&_server, dtls);
                    list_append(&dtls_list, dtls);
                }

                for (dtls = list_head(&dtls_list); dtls; dtls = list_next(&dtls_list, dtls))
                {
                    if (FD_ISSET(dtls->fd, &readset))
                    {
                        recvlen = dtls_recv(dtls, buffer, 256);
                        if (recvlen <= 0)
                        {
                            derror("recvlen = %d", recvlen);
                            list_remove(&dtls_list, dtls);
                            dtls_client_uninit(dtls);
                            free(dtls);
                            break;
                        }

                        dprint("rx (%s) from ip(%s) port(%d)", buffer,
                            dtls->remote.ipv4, dtls->remote.port);

                        sendlen = dtls_send(dtls, buffer, recvlen);
                        if (sendlen <= 0)
                        {
                            derror("send echo back len = %d", sendlen);
                            list_remove(&dtls_list, dtls);
                            dtls_client_uninit(dtls);
                            free(dtls);
                            break;
                        }
                    }
                }
            }
        }

        list_clean(&dtls_list);

        dtls_server_uninit(&_server);
    }
    else if (argc >= 3)
    {
        /////////////////////////////////////////////////////////////
        //  Client Mode
        //  ./test_dtls [remote_ip] [remote_port] [local_port]
        //
        //  for example:
        //  ./test_dtls 192.168.20.10 50000 40000
        //  ./test_dtls 192.168.20.10 50000 -> local_port is random
        /////////////////////////////////////////////////////////////

        int remote_port = atoi(argv[2]);
        int local_port = DTLS_PORT_ANY;

        if (argc == 4)
        {
            local_port = atoi(argv[3]);
        }

        tDtls client = {};
        dtls_client_init(&client, (char*)argv[1], remote_port, local_port,
                         "certs/client-cert.pem", "certs/client-key.pem", 10);

        char buffer[256];
        int sendlen;
        int i;
        int recvlen;

        for (i=0 ; i<50 && _running; i++)
        {
            sprintf(buffer, "data %d", i);
            sendlen = dtls_send(&client, buffer, strlen(buffer)+1);
            if (sendlen <= 0)
            {
                derror("sendlen = %d", sendlen);
                break;
            }

            sleep(1);

            recvlen = dtls_recv(&client, buffer, 256);
            if (recvlen <= 0)
            {
                derror("recv echo len = %d", recvlen);
                break;
            }
            dprint("recv echo = %s", buffer);
        }

        dtls_client_uninit(&client);
    }

    return 0;
}
