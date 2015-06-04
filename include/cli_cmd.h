#ifndef _CLI_CMD_H_
#define _CLI_CMD_H_

#include <stdbool.h>

#include "array.h"
#include "tcp.h"

#define CMD_OK (0)
#define CMD_FAIL (-1)

#define CMD_MAX_INPUT_SIZE (100)
#define CMD_MAX_CONN_NUM (100)

#define DEFUN(funcname, cmdcfgname, cmdstr, ...) \
    static int funcname(struct cli* cli, int argc, char* argv[]);\
    struct cli_cmd_cfg cmdcfgname = \
    { \
        .cmd_str = (char*)cmdstr, \
        .func = funcname, \
        .descs = (char*[]){__VA_ARGS__, NULL}, \
    };\
    static int funcname(struct cli* cli, int argc, char* argv[])

struct cli;
typedef int (*cli_func)(struct cli* cli, int argc, char* argv[]);

struct cli_cmd_cfg
{
    char* cmd_str;
    cli_func func;
    char** descs;
};

enum
{
    CLI_ST_NONE,
    CLI_ST_WAIT_FOR_USERNAME,
    CLI_ST_WAIT_FOR_PASSWORD,
    CLI_ST_CONNECTED,

    CLI_STATES
};

struct cli
{
    int fd; // the same value in the struct tcp's fd
    struct tcp tcp;

    int state;
};

int cli_excute_cmd(int node_id, char* string);
struct array* cli_get_completions(int node_id, char* string, char** output);
void cli_show_cmds(int node_id);

void cli_install_regular(cli_func func);
int cli_install_node(int id, char* prompt);
int cli_install_cmd(int node_id, struct cli_cmd_cfg* cfg);

int cli_sys_init(void);
void cli_sys_uninit(void);

int cli_server_init(char* username, char* password, int port, int* server_fd);
void cli_server_uninit(void);

int cli_server_accept(struct cli* cli);
void cli_uninit(struct cli* cli);

int cli_process(struct cli* cli);

#endif //_CLI_CMD_H_
