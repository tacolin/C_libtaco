#ifndef _CLI_H_
#define _CLI_H_

#include "history.h"
#include "tcp.h"

#define CLI_OK (0)
#define CLI_FAIL (-1)

#define MAX_CLI_CMD_SIZE (100)
#define MAX_CLI_CMD_HISTORY_NUM (10)
#define CLI_RETRY_COUNT (2)

struct cli;

typedef int (*cli_func)(struct cli* cli, int argc, char* argv[]);

enum
{
    CLI_ST_LOGIN,
    CLI_ST_PASSWORD,
    CLI_ST_NORMAL,

    CLI_STATES
};

struct cli
{
    char  cmd[MAX_CLI_CMD_SIZE+1];
    int   len;
    char* oldcmd;
    int   oldlen;

    char* prompt;

    int cursor;

    int is_option;
    int esc;
    int pre;
    int insert_mode;
    int is_delete;
    int in_history;

    char lastchar;

    char* banner;
    char* username;
    char* password;
    int count;

    int state;

    struct history* history;
    int history_idx;

    int fd;
    struct tcp tcp;
    int is_init;
};

struct cli_server
{
    int fd;
    struct tcp_server tcp_server;
    int is_init;

    char* banner;
    char* username;
    char* password;
};

// connection
int cli_server_init(struct cli_server* server, char* ipv4, int local_port, int max_conn_num, char* username, char* password);
int cli_server_uninit(struct cli_server* server);
int cli_server_accept(struct cli_server* server, struct cli* cli);
int cli_server_set_banner(struct cli_server* server, char* banner);

int cli_uninit(struct cli* cli);

int cli_process(struct cli* cli);

int cli_recv(struct cli* cli, void* buffer, int buffer_size);
int cli_send(struct cli* cli, void* data, int data_len);

// command
// int cli_server_set_regular(struct cli_server* server, cli_func regular_fn);
// int cli_server_install_node(struct cli_server* server, int id, char* prompt);
// int cli_server_install_cmd(struct cli_server* server, int node_id, struct cli_cmd_cfg* cfg);

// int cli_execute_cmd(struct cli* cli, int node_id, char* string);
// struct array* cli_get_completions(struct cli* cli, int node_id, char* string, char** output);

#endif //_CLI_H_
