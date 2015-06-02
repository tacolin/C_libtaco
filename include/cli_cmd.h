#ifndef _CLI_CMD_H_
#define _CLI_CMD_H_

#include "list.h"
#include "stdbool.h"

#define CMD_OK (0)
#define CMD_FAIL (-1)

struct cli_cmd;

typedef int (*cli_func)(struct cli_cmd* cmd, void* cli, int argc, char* argv[]);

struct cli_elem
{
    char* elem_str;
    char* desc;

    bool  is_option;
    bool  is_alternative;
};

struct cli_cmd
{
    char* cmd_str;
    cli_func func;
    struct list elem_list;
};

struct cli_cmd_cfg
{
    char* cmd_str;
    cli_func func;
    char** descs;
};

struct cli_node
{
    char* name;
    char* prompt;

    struct list cmd_list;
};

void cli_show_cmds(char* node_name);
int cli_install_cmd_ex(char* node_name, char* string, cli_func func, char** descs);
int cli_install_cmd(char* node_name, struct cli_cmd_cfg* cfg);
int cli_install_node(char* name, char* prompt);

int cli_set_default_node(char* node_name);

int cli_sys_init(void);
void cli_sys_uninit(void);

#define DEFUN(funcname, cmdname, cmdstr, ...) \
    static int funcname(struct cli_cmd* cmd, void* cli, int argc, char* argv[]);\
    struct cli_cmd_cfg cmdname = \
    { \
        .cmd_str = (char*)cmdstr, \
        .func = funcname, \
        .descs = (char*[]){__VA_ARGS__, NULL}, \
    }; \
    static int funcname(struct cli_cmd* cmd, void* cli, int argc, char* argv[])

#endif //_CLI_CMD_H_
