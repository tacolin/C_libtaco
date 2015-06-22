#include <stdio.h>
#include <signal.h>
#include <sys/select.h>

#include "basic.h"
#include "cli_server.h"

static int _running = 1;

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    dprint("stop main()");
}

static int _regular(struct cli* cli, int argc, char* argv[])
{
    cli_print(cli, "not found 1");
    cli_error(cli, "not found 2 : %s", argv[0]);
    return CLI_OK;
}

DEFUN(_show_me_the_money,
      _show_me_the_money_cmd,
      "show me the money",
      "desc 1",
      "desc 2",
      "desc 3",
      "desc 4")
{
    cli_print(cli, "your money is too less to be shown");
    return CLI_OK;
}

DEFUN(_show_me_the_cash,
      _show_me_the_cash_cmd,
      "show me the cash",
      "desc 1",
      "desc 2",
      "desc 3",
      "desc 4")
{
    cli_print(cli, "your cash is too less to be shown");
    return CLI_OK;
}

DEFUN(_input_id,
      _input_id_cmd,
      "input id <1-500>",
      "this is input",
      "this is id",
      "this is id value")
{
    cli_print(cli, "id value = %s", argv[0]);
    return CLI_OK;
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    struct cli_server server = {};
    struct cli cli = {};

    // cli_server_init(&server, NULL, 5000, 10, "5566", "i am sad");
    // cli_server_init(&server, NULL, 5000, 10, "5566", NULL);
    cli_server_init(&server, NULL, 5000, 10, NULL, NULL);
    cli_server_banner(&server,
                          "+---------------------+\n\r"
                          "|  This is My Banner  |\n\r"
                          "+---------------------+");

    cli_server_regular_func(&server, _regular);
    cli_server_install_node(&server, 100, "[taco]$ ");

    cli_server_install_cmd(&server, 100, &_show_me_the_money_cmd);
    cli_server_install_cmd(&server, 100, &_show_me_the_cash_cmd);
    cli_server_install_cmd(&server, 100, &_input_id_cmd);

    fd_set readset;
    int select_ret;
    struct timeval timeout;

    while (_running)
    {
        timeout.tv_sec = 1;
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

    dprint("fuck you");

    return 0;
}
