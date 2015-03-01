#include "basic.h"
#include "dtls.h"

#include <signal.h>
#include <string.h>

static int _running = 1;
static tDtlsServer _server = {};

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    _server.accept_run = 0;
    dprint("stop main()");
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    if (argc == 2)
    {
        int port = atoi(argv[1]);

        dtls_server_init(&_server, NULL, port,
                         "certs/server-cert.pem", "certs/server-key.pem", 10);

        char buffer[256];
        int  recvlen;
        int  sendlen;

        while (_running)
        {
            tDtls dtls = {};
            dtls_server_accept(&_server, &dtls);

            while (_running)
            {
                recvlen = dtls_recv(&dtls, buffer, 256);
                if (recvlen <= 0)
                {
                    derror("recvlen = %d", recvlen);
                    break;
                }

                dprint("rx (%s) from ip(%s) port(%d)", buffer,
                    dtls.remote.ipv4, dtls.remote.port);

                sendlen = dtls_send(&dtls, buffer, recvlen);
                if (sendlen <= 0)
                {
                    derror("send echo back len = %d", sendlen);
                    break;
                }
            }

            dtls_client_uninit(&dtls);
        }

        dtls_server_uninit(&_server);
    }
    else if (argc >= 3)
    {
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
