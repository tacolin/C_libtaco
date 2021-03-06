#include "basic.h"
#include "udp.h"

#include <signal.h>
#include <string.h>

// int main(int argc, char const *argv[])
// {
//     struct udp udp;
//     udp_init(&udp, NULL, 55555);
//     char buf[256];
//     struct udp_addr remote;
//     while (1)
//     {
//         udp_recv(&udp, buf, 256, &remote);
//         dprint("buf = %s", buf);
//     }

//     udp_uninit(&udp);
//     return 0;
// }

static int _running = 1;

int main(int argc, char const *argv[])
{
    if (argc == 2)
    {
        int port = atoi(argv[1]);

        struct udp server = {};
        udp_init(&server, NULL, port);

        char buffer[256];
        int  recvlen;
        int  sendlen;
        struct udp_addr remote;

        while (_running)
        {
            recvlen = udp_recv(&server, buffer, 256, &remote);
            if (recvlen <= 0)
            {
                derror("recvlen = %d", recvlen);
                break;
            }

            dprint("rx (%s) from ip(%s) port(%d)",
                    buffer, remote.ip, remote.port);

            sendlen = udp_send(&server, remote, buffer, recvlen);
            if (sendlen <= 0)
            {
                derror("send echo back len = %d", sendlen);
                break;
            }
        }

        udp_uninit(&server);
    }
    else if (argc >= 3)
    {
        int remote_port = atoi(argv[2]);
        int local_port = UDP_PORT_ANY;

        if (argc == 4)
        {
            local_port = atoi(argv[3]);
        }

        struct udp client = {};

        udp_init(&client, NULL, local_port);

        char buffer[256];
        int sendlen;
        int i;
        int recvlen;
        struct udp_addr remote;

        snprintf(remote.ip, INET6_ADDRSTRLEN, "%s", argv[1]);
        remote.port = remote_port;

        for (i=0 ; i<100; i++)
        {
            sprintf(buffer, "data %d", i);
            sendlen = udp_send(&client, remote, buffer, strlen(buffer)+1);
            if (sendlen <= 0)
            {
                derror("sendlen = %d", sendlen);
                break;
            }

            recvlen = udp_recv(&client, buffer, 256, &remote);
            if (recvlen <= 0)
            {
                derror("recv echo len = %d", recvlen);
                break;
            }
            dprint("recv echo = %s", buffer);
        }

        udp_uninit(&client);
    }

    return 0;
}
