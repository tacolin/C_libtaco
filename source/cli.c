#include "cli.h"

static void _freeCliCmd(void* cmd)
{
    if (cmd) free(cmd);
}

tCliStatus cli_sys_init(tCliSystem* sys, int port, int max_conn_num)
{
    check_if(sys == NULL, return CLI_ERROR, "sys is null");
    check_if(max_conn_num <= 0, return CLI_ERROR, "max_conn_num = %d invalid", max_conn_num);

    tTreeStatus tree_ret = tree_init(&sys->commands, sys, _freeCliCmd, NULL);
    check_if(tree_ret != TREE_OK, return CLI_ERROR, "tree_init failed");

    int check;
    struct sockaddr_in me = {};
    me.sin_family         = AF_INET;
    me.sin_addr.s_addr    = htonl(INADDR_ANY);
    me.sin_port           = htons(port);

    sys->fd = socket(AF_INET, SOCK_STREAM, 0);
    check_if(sys->fd < 0, goto _ERROR, "sock failed");

    check = bind(sys->fd, (struct sockaddr*)&me, sizeof(me));
    check_if(check < 0, goto _ERROR, "bind failed");

    check = listen(sys->fd, max_conn_num);
    check_if(check < 0, goto _ERROR, "listen failed");

    sys->is_init = 1;

    return CLI_OK;

_ERROR:
    if (sys->fd > 0)
    {
        close(sys->fd);
        sys->fd = -1;
    }
    tree_clean(&sys->commands);
    return CLI_ERROR;
}

tCliStatus cli_sys_uninit(tCliSystem* sys)
{
    check_if(sys == NULL, return CLI_ERROR, "sys is null");
    check_if(sys->is_init != 1, return CLI_ERROR, "sys is not init yet");

    if (sys->fd > 0)
    {
        close(sys->fd);
        sys->fd = -1;
    }

    tree_clean(&sys->commands);
    sys->is_init = 0;

    return CLI_OK;
}

tCliStatus cli_sys_accept(tCliSystem* sys, tCli* cli)
{
    check_if(sys == NULL, return CLI_ERROR, "sys is null");
    check_if(cli == NULL, return CLI_ERROR, "cli is null");
    check_if(sys->is_init != 1, return CLI_ERROR, "sys is not init yet");
    check_if(sys->fd <= 0, return CLI_ERROR, "sys fd = %d invalid", sys->fd);

    int fd;
    struct sockaddr_in remote = {};
    int addrlen = sizeof(remote);

    fd = accept(sys->fd, (struct sockaddr*)&remote, &addrlen);
    check_if(fd < 0, return CLI_ERROR, "accept failed");

    int check = inet_ntop(AF_INET, &remote.sin_addr, cli->srcipv4, INET_ADDRSTRLEN);
    check_if(check != 1, goto _ERROR, "inet_ntop failed");

    cli->srcport = ntohs(remote.sin_port);
    cli->fd      = fd;

    cli->is_init = 1;

    return CLI_OK;

_ERROR:
    if (fd > 0)
    {
        close(fd);
    }
    return CLI_ERROR;
}

tCliStatus cli_uninit(tCli* cli)
{
    check_if(cli == NULL, return CLI_ERROR, "cli is null");
    check_if(cli->is_init == 0, return CLI_ERROR, "cli is not init yet");

    if (cli->fd > 0)
    {
        close(cli->fd);
        cli->fd = -1;
    }

    cli->is_init = 0;

    return CLI_OK;
}

tCliStatus cli_sys_addCmd(tCliSystem* sys, char* cmd, void* arg, tCliCallback callback);

tCliStatus cli_proc(tCli* cli, char* input);

tCliStatus cli_sys_run(tCliSystem* sys);
tCliStatus cli_sys_break(tCliSystem* sys);
