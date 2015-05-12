#ifndef _CLI_H_
#define _CLI_H_

#include <stdbool.h>

#include "tcp.h"
#include "hash.h"
#include "history.h"

#define CLI_OK (0)
#define CLI_FAIL (-1)

#define MAX_CLI_CMD_SIZE (100)

struct cli
{
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
};

#endif //_CLI_H_
