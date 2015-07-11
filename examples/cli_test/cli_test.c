#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "basic.h"
#include "cli.h"

static int _running = 1;

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    dprint("stop main()");
}

static int _regular(struct cli* cli, int argc, char* argv[])
{
    cli_error(cli, "invalid input : %s", argv[0]);
    return CLI_OK;
}

DEFUN(_write_file,
      _write_file_cmd,
      "write FILEPATH",
      "write input histories into a text file",
      "filepath")
{
    cli_print(cli, "filepath = %s", argv[0]);
    cli_save_histories(cli, argv[0]);
    return CLI_OK;
}

DEFUN(_read_file,
      _read_file_cmd,
      "read FILEPATH",
      "read command from configuration file",
      "filepath")
{
    cli_print(cli, "filepath = %s", argv[0]);
    cli_read_file(cli, argv[0]);
    return CLI_OK;
}

enum
{
    EXEC_NODE = 1,
    ENABLE_NODE = 2,
    CONFIG_NODE = 3,
};

struct data
{
    int dst_port;
    char dst_ip[INET_ADDRSTRLEN];
    char dst_host_name[100];
};

static struct data _taco_data = {};

DEFUN(_enable,
      _enable_cmd,
      "enable",
      "change to enable node")
{
    cli_change_node(cli, ENABLE_NODE);
    return CLI_OK;
}

DEFUN(_config,
      _config_cmd,
      "config terminal",
      "change to configure node")
{
    cli_change_node(cli, CONFIG_NODE);
    return CLI_OK;
}

DEFUN(_exit_node,
      _exit_node_cmd,
      "exit",
      "exit current node")
{
    if (cli->node_id == EXEC_NODE)
    {
        // cli should be uninit / disconnected when return CLI_BREAK
        return CLI_BREAK;
    }
    else if (cli->node_id == ENABLE_NODE)
    {
        cli_change_node(cli, EXEC_NODE);
    }
    else if (cli->node_id == CONFIG_NODE)
    {
        cli_change_node(cli, ENABLE_NODE);
    }
    return CLI_OK;
}

DEFUN(_show_info,
      _show_info_cmd,
      "show info",
      "show command",
      "information")
{
    cli_print(cli, "current info is :");
    cli_print(cli, "dst port = %d", _taco_data.dst_port);
    cli_print(cli, "dst ipv4 = %s", _taco_data.dst_ip);
    cli_print(cli, "dst hostname = %s", _taco_data.dst_host_name);
    cli_print(cli, "");
    return CLI_OK;
}

DEFUN(_system_enable_disable,
      _system_enable_disable_cmd,
      "system (enable | disable)",
      "system control",
      "enable or disable")
{
    if (argc != 1)
    {
        cli_error(cli, "argc = %d invalid", argc);
        return CLI_OK;
    }

    if (0 == strcmp(argv[0], "enable"))
    {
        cli_print(cli, "system is enabled now");
    }
    else if (0 == strcmp(argv[0], "disable"))
    {
        cli_print(cli, "system is disabled now");
    }
    else
    {
        cli_error(cli, "input string = %s invalid", argv[0]);
    }
    return CLI_OK;
}

DEFUN(_set_dst_port,
      _set_dst_port_cmd,
      "set dst port <1-65535>",
      "set command",
      "destination",
      "port",
      "port number is integer")
{
    if (argc != 1)
    {
        cli_error(cli, "argc = %d invalid", argc);
        return CLI_OK;
    }
    _taco_data.dst_port = atoi(argv[0]);
    cli_print(cli, "input dst_port = %d", _taco_data.dst_port);
    return CLI_OK;
}

DEFUN(_set_dst_ip,
      _set_dst_ip_cmd,
      "set dst ipv4 A.B.C.D",
      "set command",
      "destination",
      "ipv4",
      "ipv4 string")
{
    if (argc != 1)
    {
        cli_error(cli, "argc = %d invalid", argc);
        return CLI_OK;
    }
    snprintf(_taco_data.dst_ip, INET_ADDRSTRLEN, "%s", argv[0]);
    cli_print(cli, "input dst_ip = %s", _taco_data.dst_ip);
    return CLI_OK;
}

DEFUN(_set_host_name,
      _set_host_name_cmd,
      "set dst hostname NAME",
      "set command",
      "destination",
      "hostname",
      "name string")
{
    if (argc != 1)
    {
        cli_error(cli, "argc = %d invalid", argc);
        return CLI_OK;
    }
    snprintf(_taco_data.dst_host_name, 100, "%s", argv[0]);
    cli_print(cli, "input dst_host_name = %s", _taco_data.dst_host_name);
    return CLI_OK;
}

DEFUN(_set_total,
      _set_total_cmd,
      "set dst port <1-65535> ipv4 A.B.C.D hostname NAME",
      "set command",
      "destination",
      "port",
      "port number",
      "ipv4",
      "ipv4 string",
      "hostname",
      "name string")
{
    if (argc != 3)
    {
        cli_error(cli, "argc = %d invalid", argc);
        return CLI_OK;
    }

    _taco_data.dst_port = atoi(argv[0]);
    snprintf(_taco_data.dst_ip, INET_ADDRSTRLEN, "%s", argv[1]);
    snprintf(_taco_data.dst_host_name, 100, "%s", argv[2]);
    cli_print(cli, "set total ok");
    return CLI_OK;
}

static void _cli_setup(struct cli_server* server)
{
    CHECK_IF(server == NULL, return, "server is null");

    // cli_server_init(server, NULL, 5000, 10, /*username*/"5566", /*password*/"imsad");
    cli_server_init(server, NULL, 5000, 10, NULL, NULL);

    char banner[] = "+---------------------+\n\r"
                    "|  This is My Banner  |\n\r"
                    "+---------------------+";
    cli_server_banner(server, banner);

    cli_server_regular_func(server, _regular);

    cli_server_install_node(server, EXEC_NODE, "[taco]> ");
    cli_server_install_cmd(server, EXEC_NODE, &_enable_cmd);
    cli_server_install_cmd(server, EXEC_NODE, &_write_file_cmd);
    cli_server_install_cmd(server, EXEC_NODE, &_read_file_cmd);
    cli_server_install_cmd(server, EXEC_NODE, &_exit_node_cmd);

    cli_server_install_cmd(server, EXEC_NODE, &_show_info_cmd);

    cli_server_install_node(server, ENABLE_NODE, "[taco]# ");
    cli_server_install_cmd(server, ENABLE_NODE, &_config_cmd);
    cli_server_install_cmd(server, ENABLE_NODE, &_write_file_cmd);
    cli_server_install_cmd(server, ENABLE_NODE, &_read_file_cmd);
    cli_server_install_cmd(server, ENABLE_NODE, &_exit_node_cmd);

    cli_server_install_cmd(server, ENABLE_NODE, &_system_enable_disable_cmd);

    cli_server_install_node(server, CONFIG_NODE, "[taco](config)# ");
    cli_server_install_cmd(server, CONFIG_NODE, &_write_file_cmd);
    cli_server_install_cmd(server, CONFIG_NODE, &_read_file_cmd);
    cli_server_install_cmd(server, CONFIG_NODE, &_exit_node_cmd);

    cli_server_install_cmd(server, CONFIG_NODE, &_set_dst_port_cmd);
    cli_server_install_cmd(server, CONFIG_NODE, &_set_dst_ip_cmd);
    cli_server_install_cmd(server, CONFIG_NODE, &_set_host_name_cmd);
    cli_server_install_cmd(server, CONFIG_NODE, &_set_total_cmd);

    return;
}

int main(int argc, char const *argv[])
{
    struct cli_server server = {};
    struct cli cli = {};

    _cli_setup(&server);

    fd_set readset;
    int select_ret;
    struct timeval timeout;

    while (_running)
    {
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(server.fd, &readset);

        if (cli.is_init)
        {
            FD_SET(cli.fd, &readset);
        }

        select_ret = select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
        if (select_ret < 0)
        {
            derror("select_ret = %d", select_ret);
            break;
        }
        else if (select_ret == 0)
        {
            continue;
        }
        else
        {
            if (FD_ISSET(server.fd, &readset))
            {
                cli_server_accept(&server, &cli);
            }

            if (cli.is_init)
            {
                if (FD_ISSET(cli.fd, &readset))
                {
                    if (CLI_OK != cli_process(&cli))
                    {
                        break;
                    }
                }
            }
        }
    }

    cli_uninit(&cli);
    cli_server_uninit(&server);

    dprint("cli_test end");
    return 0;
}
