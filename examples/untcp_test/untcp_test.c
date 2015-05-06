#include "basic.h"
#include "unsock.h"

#include <signal.h>
#include <string.h>

static int _running = 1;
static struct untcp_server _server = {};

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    untcp_server_uninit(&_server);
    dprint("stop main()");
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    if (argc == 2)
    {
        untcp_server_init(&_server, (char*)argv[1], 10);

        char buffer[256];
        int  recvlen;
        int  sendlen;

        while (_running)
        {
            struct untcp untcp = {};
            untcp_server_accept(&_server, &untcp);

            while (1)
            {
                recvlen = untcp_recv(&untcp, buffer, 256);
                if (recvlen <= 0)
                {
                    derror("recvlen = %d", recvlen);
                    break;
                }

                dprint("rx (%s)", buffer);

                sendlen = untcp_send(&untcp, buffer, recvlen);
                if (sendlen <= 0)
                {
                    derror("send echo back len = %d", sendlen);
                    break;
                }
            }

            untcp_client_uninit(&untcp);
        }

        untcp_server_uninit(&_server);
    }
    else if (argc == 3)
    {
        struct untcp client = {};
        untcp_client_init(&client, (char*)argv[1]);

        char buffer[256];
        int sendlen;
        int i;
        int recvlen;
        int datanum = atoi(argv[2]);

        for (i=0 ; i<datanum; i++)
        {
            sprintf(buffer, "data %d", i);
            sendlen = untcp_send(&client, buffer, strlen(buffer)+1);
            if (sendlen <= 0)
            {
                derror("sendlen = %d", sendlen);
                break;
            }

            recvlen = untcp_recv(&client, buffer, 256);
            if (recvlen <= 0)
            {
                derror("recv echo len = %d", recvlen);
                break;
            }
            dprint("recv echo = %s", buffer);
        }

        untcp_client_uninit(&client);
    }

    return 0;
}
