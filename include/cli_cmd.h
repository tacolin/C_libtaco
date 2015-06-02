#ifndef _CLI_CMD_H_
#define _CLI_CMD_H_

#include "list.h"
#include "stdbool.h"

#define CMD_OK (0)
#define CMD_FAIL (-1)

// #define DEFUN(funcname, cmdname, cmdstr, ...) \
//     static int funcname(struct cli_cmd* cmd, struct cli* cli, int argc, char* argv[]);\
//     struct cli_cmd_cfg cmdname = \
//     { \
//         .cmd_str = (char*)cmdstr, \
//         .func = funcname, \
//         .descs = (char*[]){__VA_ARGS__, NULL}, \
//     }; \
//     static int funcname(struct cli_cmd* cmd, struct cli* cli, int argc, char* argv[])

struct cli;
struct cli_cmd;

typedef int (*cli_func)(struct cli_cmd* cmd, struct cli* cli, int argc, char* argv[]);

enum
{
    CMD_TOKEN,
    CMD_INT,
    CMD_IPV4,
    CMD_MACADDR,
    CMD_STRING,

    CMD_TYPES
};

struct cli_cmd
{
    int type;
    char* str;
    char* desc;
    cli_func func;
    struct list sub_cmds;
};

struct cli_node
{
    int id;
    char* prompt;
    struct list cmds;
};

void cli_show_cmds(int node_id);

void cli_install_regular(cli_func func);

struct cli_node* cli_install_node(int id, char* prompt);
struct cli_cmd*  cli_install_cmd(struct cli_cmd* parent, char* string, cli_func func, int node_id, char* desc);

int cli_sys_init(void);
void cli_sys_uninit(void);

#endif //_CLI_CMD_H_
