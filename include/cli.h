#ifndef _CLI_H_
#define _CLI_H_

#include <stdbool.h>

#include "tcp.h"
#include "hash.h"
#include "history.h"

#define CLI_OK (0)
#define CLI_FAIL (-1)

#define MAX_CLI_CMD_SIZE (100)
#define MAX_CLI_CMD_NUM  (100)

struct cli_mode
{
    char prompt[MAX_CLI_CMD_SIZE+1];
    struct hash cmds;
};

struct cli
{
    struct tcp tcp;

    char  cmd[MAX_CLI_CMD_SIZE+1];
    int   len;
    char* oldcmd;
    int   oldlen;

    int cursor;

    bool option;
    bool esc;
    bool pre;
    bool insert_mode;
    bool is_delete;
    bool in_history;

    char lastchar;
    struct history history;

    struct cli_mode* curr_mode;
};

struct cli_system
{
    struct tcp_server server;
    int local_port;
    int max_cli_num;

    struct list modes;
    struct list clis;

    bool running;
};

struct cli_cmd
{
    void (*execfn)(int argc, char* argv[], void* arg);
    void* arg;

    struct hash subcmds;
};

int cli_sys_init(struct cli_system* sys, int local_port, int max_cli_num);
void cli_sys_uninit(struct cli_system* sys);

int cli_sys_set_prompt(struct cli_system* sys, char* prompt);
int cli_sys_add_cmd(struct cli_system* sys, char* cmdstr, void (*execfn)(int argc, char* argv[], void* arg), void* arg);
int cli_sys_accpet(struct cli_system* sys, struct cli* cli);

#endif //_CLI_H_
