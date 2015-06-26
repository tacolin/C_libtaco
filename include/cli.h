#ifndef _CLI_H_
#define _CLI_H_

#include "array.h"
#include "history.h"
#include "tcp.h"

#define CLI_OK (0)
#define CLI_FAIL (-1)

#define CLI_MAX_CMD_SIZE (100)
#define CLI_MAX_CMD_HISTORY_NUM (10)
#define CLI_MAX_RETRY_NUM (2)

////////////////////////////////////////////////////////////////////////////////

struct cli;
struct cli_node;

typedef int (*cli_func)(struct cli* cli, int argc, char* argv[]);

struct cli_cmd_cfg
{
    char* cmd_str;
    cli_func func;
    char** descs;
};

struct cli
{
    int fd;

    ////////////////////////////////////////////////////////////////////////////

    char  cmd[CLI_MAX_CMD_SIZE+1];
    int   len;
    char* oldcmd;
    int   oldlen;

    char* prompt;

    int cursor;

    int is_option;
    int esc;
    int insert_mode;
    int is_delete;
    int in_history;

    char lastchar;

    char* banner;
    char* username;
    char* password;
    int count;

    struct array* nodes;
    cli_func regular;

    int node_id;

    int state;

    struct history* history;
    int history_idx;

    struct tcp tcp;
    int is_init;
};

struct cli_server
{
    int fd;

    ////////////////////////////////////////////////////////////////////////////

    struct tcp_server tcp_server;
    int is_init;

    char* banner;
    char* username;
    char* password;

    struct array* nodes;
    cli_func regular;

    struct cli_node* default_node;
};

////////////////////////////////////////////////////////////////////////////////

int cli_server_init(struct cli_server* server, char* local_ipv4, int local_port, int max_conn_num, char* username, char* password);
int cli_server_uninit(struct cli_server* server);
int cli_server_accept(struct cli_server* server, struct cli* cli);

int cli_server_banner(struct cli_server* server, char* banner);
int cli_server_regular_func(struct cli_server* server, cli_func regular);

int cli_server_install_node(struct cli_server* server, int id, char* prompt);
int cli_server_install_cmd(struct cli_server* server, int node_id, struct cli_cmd_cfg* cfg);
int cli_server_default_node(struct cli_server* server, int id);

////////////////////////////////////////////////////////////////////////////////

int cli_uninit(struct cli* cli);
int cli_process(struct cli* cli);

int cli_change_node(struct cli* cli, int node_id);

int cli_execute_cmd(struct cli* cli, char* string);

int cli_save_histories(struct cli* cli, char* filepath);

////////////////////////////////////////////////////////////////////////////////

int cli_send(struct cli* cli, void* data, int data_len);

////////////////////////////////////////////////////////////////////////////////

#define cli_print(cli, msg, param...)\
{\
    char* __tmp;\
    asprintf(&__tmp, msg"\n\r", ##param);\
    cli_send(cli, __tmp, strlen(__tmp)+1);\
    free(__tmp);\
}

#define cli_error(cli, msg, param...)\
{\
    char* __tmp;\
    asprintf(&__tmp, "> "msg"\n\r", ##param);\
    cli_send(cli, __tmp, strlen(__tmp)+1);\
    free(__tmp);\
}

#define DEFUN(funcname, cmdcfgname, cmdstr, ...) \
    static int funcname(struct cli* cli, int argc, char* argv[]);\
    struct cli_cmd_cfg cmdcfgname = \
    {\
        .cmd_str = (char*)cmdstr, \
        .func = funcname, \
        .descs = (char*[]){__VA_ARGS__, NULL}, \
    };\
    static int funcname(struct cli* cli, int argc, char* argv[])

////////////////////////////////////////////////////////////////////////////////

#endif //_CLI_H_

