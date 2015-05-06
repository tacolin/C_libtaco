#include "basic.h"
#include "tcp.h"

#include <signal.h>
#include <string.h>

static int _running = 1;

int main(int argc, char const *argv[])
{
    if (argc == 2)
    {
        int port = atoi(argv[1]);

        struct tcp_server server = {};
        tcp_server_init(&server, NULL, port, 10);

        char buffer[256];
        int  recvlen;
        int  sendlen;

        while (_running)
        {
            struct tcp tcp = {};
            tcp_server_accept(&server, &tcp);

            while (1)
            {
                recvlen = tcp_recv(&tcp, buffer, 256);
                if (recvlen <= 0)
                {
                    derror("recvlen = %d", recvlen);
                    break;
                }

                dprint("rx (%s) from ip(%s) port(%d)", buffer,
                    tcp.remote.ipv4, tcp.remote.port);

                sendlen = tcp_send(&tcp, buffer, recvlen);
                if (sendlen <= 0)
                {
                    derror("send echo back len = %d", sendlen);
                    break;
                }
            }

            tcp_client_uninit(&tcp);
        }

        tcp_server_uninit(&server);
    }
    else if (argc >= 3)
    {
        int remote_port = atoi(argv[2]);
        int local_port = TCP_PORT_ANY;

        if (argc == 4)
        {
            local_port = atoi(argv[3]);
        }

        struct tcp client = {};
        tcp_client_init(&client, (char*)argv[1], remote_port, local_port);;

        char buffer[256];
        int sendlen;
        int i;
        int recvlen;

        for (i=0 ; i<100; i++)
        {
            sprintf(buffer, "data %d", i);
            sendlen = tcp_send(&client, buffer, strlen(buffer)+1);
            if (sendlen <= 0)
            {
                derror("sendlen = %d", sendlen);
                break;
            }

            recvlen = tcp_recv(&client, buffer, 256);
            if (recvlen <= 0)
            {
                derror("recv echo len = %d", recvlen);
                break;
            }
            dprint("recv echo = %s", buffer);
        }

        tcp_client_uninit(&client);
    }

    return 0;
}
