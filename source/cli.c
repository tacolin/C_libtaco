#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"

#define CTRL(key) (key - '@')

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static const char _negotiate[] = "\xFF\xFB\x03"
                                 "\xFF\xFB\x01"
                                 "\xFF\xFD\x03"
                                 "\xFF\xFD\x01";

void _add_def_cmds(void)
{

}

int cli_sys_init(struct cli_system* sys, int local_port, int max_cli_num)
{
    CHECK_IF(sys == NULL, return CLI_FAIL, "sys is null");
    CHECK_IF(local_port <= 0, return CLI_FAIL, "local_port = %d invalid", local_port);
    CHECK_IF(max_cli_num <= 0, return CLI_FAIL, "max_cli_num = %d invalid", max_cli_num);

    sys->max_cli_num = max_cli_num;
    sys->local_port  = local_port;
    sys->running     = false;

    tcp_server_init(&sys->server, NULL, local_port, max_cli_num);
    list_init(&sys->clis, _clean_cli);
    hash_init(&sys->defcmds, _clean_cmd, MAX_CLI_CMD_NUM);
    hash_init(&sys->cmds, _clean_cmd, MAX_CLI_CMD_NUM);
    _add_def_cmds();
    return CLI_OK;
}

void cli_sys_uninit(struct cli_system* sys)
{
    CHECK_IF(sys == NULL, return, "sys is null");

    tcp_server_uninit(&sys->server);
    list_clean(&sys->clis);
    hash_uninit(&sys->defcmds);
    hash_uninit(&sys->cmds);
    sys->max_cli_num = 0;
    sys->local_port  = 0;
    sys->running     = false;
    return;
}

int cli_sys_set_prompt(struct cli_system* sys, char* prompt)
{
    CHECK_IF(sys == NULL, return CLI_FAIL, "sys is null");
    CHECK_IF(sys->running, return CLI_FAIL, "sys is running");
    CHECK_IF(prompt == NULL, return CLI_FAIL, "prompt is null");

    int len = (strlen(prompt) > MAX_CLI_CMD_SIZE) ? MAX_CLI_CMD_SIZE : strlen(prompt);
    strncpy(sys->prompt, prompt, len+1);
    return CLI_OK;
}

int cli_sys_add_cmd(struct cli_system* sys, char* cmdstr, void (*execfn)(int argc, char* argv[], void* arg), void* arg)
{
    CHECK_IF(sys == NULL, return CLI_FAIL, "sys is null");
    CHECK_IF(sys->running, return CLI_FAIL, "sys is running");
    CHECK_IF(cmdstr == NULL, return CLI_FAIL, "cmdstr is null");
    CHECK_IF(execfn == NULL, return CLI_FAIL, "execfn is null");


}

int cli_sys_accpet(struct cli_system* sys, struct cli* cli);
