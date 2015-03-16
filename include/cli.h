#ifndef _CLI_H_
#define _CLI_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "basic.h"
#include "list.h"
#include "ringbuf.h"
#include "tree.h"

#define CLI_MAX_CMD_STR_LEN 108

#define DEFUN()

typedef enum
{
    CLI_OK = 0,
    CLI_ERROR = -1

} tCliStatus;

typedef tCliStatus (*tCliCallback)(char* cmd, void* arg);

typedef struct tCliCmd
{
    tTreeHdr hdr;

    char cmd[CLI_MAX_CMD_STR_LEN];

    void* arg;
    tCliCallback callback;

} tCliCmd;

typedef struct tCliSystem
{
    tTreeHdr hdr;

    int fd;

    tTree commands;

    int is_init;

} tCliSystem;

typedef struct tCli
{
    int fd;

    tRingBuf   input_history;
    tCliSystem *system;

    char srcipv4[INET_ADDRSTRLEN];
    int  srcport;

    int is_init;

} tCli;

tCliStatus cli_sys_init(tCliSystem* sys, int port, int max_conn_num);
tCliStatus cli_sys_uninit(tCliSystem* sys);

tCliStatus cli_sys_addCmd(tCliSystem* sys, char* cmd, void* arg, tCliCallback callback);

tCliStatus cli_sys_accept(tCliSystem* sys, tCli* cli);
tCliStatus cli_uninit(tCli* cli);

tCliStatus cli_proc(tCli* cli, char* input);

tCliStatus cli_sys_run(tCliSystem* sys);
tCliStatus cli_sys_break(tCliSystem* sys);

#endif //_CLI_H_
