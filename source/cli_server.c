#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_server.h"

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

int cli_server_init(struct cli_server* server, char* ipv4, int local_port, int max_conn_num, char* username, char* password)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");

    memset(server, 0, sizeof(struct cli_server));

    int chk = tcp_server_init(&server->tcp_server, ipv4, local_port, max_conn_num);
    CHECK_IF(chk != TCP_OK, return CLI_FAIL, "tcp_server_init failed");

    server->fd      = server->tcp_server.fd;

    if (username)
    {
        server->username = strdup(username);
        if (password)
        {
            server->password = strdup(password);
        }
    }
    server->is_init = 1;
    return CLI_OK;
}

int cli_server_uninit(struct cli_server* server)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(server->fd <= 0, return CLI_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(server->is_init != 1, return CLI_FAIL, "server is not init yet");

    tcp_server_uninit(&server->tcp_server);
    server->fd = -1;
    server->is_init = 0;

    if (server->username)
    {
        free(server->username);
        server->username = NULL;
    }

    if (server->password)
    {
        free(server->password);
        server->password = NULL;
    }

    if (server->banner)
    {
        free(server->banner);
        server->banner = NULL;
    }

    return CLI_OK;
}

int cli_server_set_banner(struct cli_server* server, char* banner)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(banner == NULL, return CLI_FAIL, "banner is null");
    CHECK_IF(server->fd <= 0, return CLI_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(server->is_init != 1, return CLI_FAIL, "server is not init yet");

    server->banner = strdup(banner);
    return CLI_OK;
}

static void _show_prompt(struct cli* cli)
{
    cli_send(cli, cli->prompt, strlen(cli->prompt));
}

static void _pre_process(struct cli* cli)
{
    if (cli->oldcmd != NULL) // 把上一次的cmd變成現在的cmd
    {
        cli->len    = cli->oldlen;
        cli->cursor = cli->oldlen;
        cli->pre    = 1;
        cli->oldcmd = NULL;
        cli->oldlen = 0;
    }
    else
    {
        memset(cli->cmd, 0, MAX_CLI_CMD_SIZE+1);
        cli->len    = 0;
        cli->cursor = 0;
    }
    _show_prompt(cli);
    cli->pre = 0;
    return;
}

static void _show_banner(struct cli* cli)
{
    if (cli->banner)
    {
        cli_send(cli, "\r\n", 2);
        cli_send(cli, cli->banner, strlen(cli->banner));
        cli_send(cli, "\r\n", 2);
    }
}

int cli_server_accept(struct cli_server* server, struct cli* cli)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(server->fd <= 0, return CLI_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(server->is_init != 1, return CLI_FAIL, "server is not init yet");
    CHECK_IF(cli == NULL, return CLI_FAIL, "cli is null");

    memset(cli, 0, sizeof(struct cli));

    int chk = tcp_server_accept(&server->tcp_server, &cli->tcp);
    CHECK_IF(chk != TCP_OK, return CLI_FAIL, "tcp_server_accept failed");

    cli->history     = history_create(MAX_CLI_CMD_SIZE+1, MAX_CLI_CMD_HISTORY_NUM);
    cli->pre         = 1;
    cli->insert_mode = 1;
    cli->fd          = cli->tcp.fd;
    cli->banner      = server->banner;
    cli->username    = server->username;
    cli->password    = server->password;
    cli->is_init     = 1;
    cli->state       = (cli->username == NULL) ? CLI_ST_NORMAL : CLI_ST_LOGIN ;

    if (cli->state == CLI_ST_NORMAL)
    {
        cli->prompt = strdup("[taco]$ ");
        _show_banner(cli);
        cli_send(cli, "\r\n", 2);
    }
    else if (cli->state == CLI_ST_LOGIN)
    {
        cli->prompt = strdup("UserName: ");
    }

    const char negotiate[] = "\xFF\xFB\x03"
                             "\xFF\xFB\x01"
                             "\xFF\xFD\x03"
                             "\xFF\xFD\x01";

    cli_send(cli, (void*)negotiate, strlen(negotiate));

    _pre_process(cli);

    return CLI_OK;
}

static void _print_history(int idx, void* data)
{
    derror("[%d] : %s", idx, (char*)data);
}

int cli_uninit(struct cli* cli)
{
    CHECK_IF(cli == NULL, return CLI_FAIL, "cli is null");
    CHECK_IF(cli->is_init != 1, return CLI_FAIL, "cli is not init yet");
    CHECK_IF(cli->fd <= 0, return CLI_FAIL, "cli fd = %d invalid", cli->fd);

    tcp_client_uninit(&cli->tcp);

    if (cli->history)
    {
        history_do_all(cli->history, _print_history);
        history_release(cli->history);
        cli->history = NULL;
    }

    if (cli->prompt)
    {
        free(cli->prompt);
        cli->prompt = NULL;
    }
    cli->fd = -1;
    cli->is_init = 0;
    return CLI_OK;
}

int cli_recv(struct cli* cli, void* buffer, int buffer_size)
{
    CHECK_IF(cli == NULL, return -1, "cli is null");

    return tcp_recv(&cli->tcp, buffer, buffer_size);
}

int cli_send(struct cli* cli, void* data, int data_len)
{
    CHECK_IF(cli == NULL, return -1, "cli is null");
    CHECK_IF(data_len <= 0, return -1, "data_len = %d invalid", data_len);

    return tcp_send(&cli->tcp, data, data_len);
}

enum
{
    PROC_CONT,
    PROC_PRE_CONT,
    PROC_ERROR,
    PROC_NONE,

    PROC_RETURN_VALUES
};

static int _proc_option(struct cli* cli, unsigned char c)
{
    if ((c == 255) && (cli->is_option == 0))
    {
        // 收到 255  表示下一次收到的 byte 是拿來做 option 的，先標記起來
        cli->is_option++;
        return PROC_CONT;
    }

    if (cli->is_option)
    {
        if ((c >= 251) && (c <= 254))
        {
            // 如果收到 251~254  表示 option 還沒結束  繼續
            cli->is_option = c;
            return PROC_CONT;
        }
        else if (c != 255)
        {
            // 如果不是 251~254，也不是 255，那就表示 opt 結束了
            cli->is_option = 0;
            return PROC_CONT;
        }

        // 收到 255 第二次，表示要處理之前收到的資料
        cli->is_option = 0;
    }
    return PROC_NONE;
}

static void _del_one_char(struct cli* cli)
{
    if (cli->len == cli->cursor)
    {
        cli->cmd[--(cli->cursor)] = 0;
        cli_send(cli, "\b \b", 3);
    }
    else
    {
        // 東湊西湊湊出 delete ...
        // 先把自己儲存的 cmd 更新 (刪除某一個char)
        int i;
        for (i=cli->cursor; i<=cli->len; i++)
        {
            cli->cmd[i] = cli->cmd[i+1];
        }
        // 這一段主要是去更新跟清除 telnet client 那邊螢幕上的游標跟字
        // 詳細的可以自己體會 ...(因為說明很麻煩 ?)
        if (strlen(cli->cmd + cli->cursor) > 0)
        {
            cli_send(cli, cli->cmd + cli->cursor, strlen(cli->cmd + cli->cursor));
        }
        cli_send(cli, " ", 1);
        for (i=0; i<= (int)strlen(cli->cmd + cli->cursor); i++)
        {
            cli_send(cli, "\b", 1);
        }
    }
    cli->len--;
}

static void _clear_line(struct cli* cli)
{
    if (cli->len <= 0) return;

    unsigned char data[cli->len + 1];

    data[cli->len] = '\0';

    memset(data, '\b', cli->cursor);
    cli_send(cli, data, cli->cursor);

    memset(data, ' ', cli->len);
    cli_send(cli, data, cli->len);

    memset(data, '\b', cli->len);
    cli_send(cli, data, cli->len);

    memset(cli->cmd, 0, cli->len);
    cli->len    = 0;
    cli->cursor = 0;
    return;
}

static void _change_curr_cmd(struct cli* cli, char* newcmd)
{
    derror("valid 1");
    _clear_line(cli);
    derror("valid 2");

    snprintf(cli->cmd, MAX_CLI_CMD_SIZE, "%s", newcmd);

    cli->len = strlen(cli->cmd);
    cli->cursor = strlen(cli->cmd);

    cli_send(cli, cli->cmd, cli->len);
}

static int _proc_ctrl(struct cli* cli, unsigned char *c)
{
    if (*c == 27)
    {
        // 如果收到 27 表示是 ESC 控制字元，標記起來，等待下一個 byte
        cli->esc = 1;
        return PROC_CONT;
    }

    // 如果上一個收到的是 esc 控制字元的話
    if (cli->esc)
    {
        // 處理上下左右箭頭鍵
        if (cli->esc == '[')
        {
            cli->esc = 0;

            switch (*c)
            {
                case 'A':
                    *c = CTRL('P');
                    break;

                case 'B':
                    *c = CTRL('N');
                    break;

                case 'C':
                    *c = CTRL('F');
                    break;

                case 'D':
                    *c = CTRL('B');
                    break;

                case '3':
                    cli->is_delete = 1;
                    return PROC_CONT;
                    // break;

                default:
                    *c = 0;
                    break;
            }
        }
        else // 如果不是 [ 的控制字元我不想理會  (原因不明)
        {
            cli->esc = (*c == '[') ? *c : 0;
            return PROC_CONT;
        }
    }

    if (cli->is_delete)
    {
        // 只刪一個 char
        if (*c == 126 && cli->cursor != cli->len)
        {
            _del_one_char(cli);
        }
        cli->is_delete = 0;
        return PROC_CONT;
    }

    if (*c == CTRL('C'))
    {
        // 收到 ctrl c 那就回傳 alert，對方就會聽到「噹」一聲
        cli_send(cli, "\a", 1);
        cli_send(cli, "\r\n", 2);
        return PROC_PRE_CONT;
    }

    //  backword          backspace         delete
    if (*c == CTRL('W') || *c == CTRL('H') || *c == 0x7f)
    {
        int back = 0;

        if (*c == CTRL('W'))
        {
            if (cli->len == 0) return PROC_CONT;

            if (cli->cursor == 0) return PROC_CONT;

            int nc = cli->cursor;
            while (nc && cli->cmd[nc-1] == ' ') // calculate back redundant space
            {
                nc--;
                back++;
            }

            while (nc && cli->cmd[nc-1] != ' ') // calculate back word
            {
                nc--;
                back++;
            }
        }
        else
        {
            // 如果 backspace 或 delete 已經退到頭了，那就會響警示音
            if (cli->len == 0 || cli->cursor == 0)
            {
                cli_send(cli, "\a", 1);
                return PROC_CONT;
            }
            back = 1;
        }

        if (back)
        {
            while (back)
            {
                _del_one_char(cli);
                back--;
            }
            return PROC_CONT;
        }
    }

    if (*c == CTRL('L'))
    {
        // CTRL L - Redraw ?
        int i;
        int cursor_back = cli->len - cli->cursor;

        cli_send(cli, "\r\n", 2);
        cli_send(cli, cli->prompt, strlen(cli->prompt));

        if (cli->len > 0)
        {
            cli_send(cli, cli->cmd, cli->len);
        }

        for (i=0; i<cursor_back; i++)
        {
            cli_send(cli, "\b", 1);
        }
        return PROC_CONT;
    }

    if (*c == CTRL('U'))
    {
        _clear_line(cli);
        return PROC_CONT;
    }

    if (*c == CTRL('K'))
    {
        // Kill to the EOL
        if (cli->cursor == cli->len) return PROC_CONT;

        int i;
        for (i=cli->cursor; i<cli->len; i++)
        {
            cli_send(cli, " ", 1);
        }

        for (i=cli->cursor; i<cli->len; i++)
        {
            cli_send(cli, "\b", 1);
        }

        memset(cli->cmd + cli->cursor, 0, cli->len - cli->cursor);
        cli->len = cli->cursor;
        return PROC_CONT;
    }

    if (*c == CTRL('D'))
    {
        // EOT
        if (cli->len > 0)
        {
            return PROC_CONT;
        }
        cli->len = -1;
        return PROC_ERROR;
    }

    if (*c == CTRL('Z'))
    {
        // disable (?)
        return PROC_PRE_CONT;
    }

    if (*c == CTRL('I'))
    {
        // TAB
        if (cli->cursor != cli->len) return PROC_CONT;

        int num_completions = 0;

        if (num_completions == 0)
        {
            cli_send(cli, "\a", 1);
        }
        else if (num_completions == 1)
        {
            // show one complete
            // TODO
        }
        else if (num_completions > 1)
        {
            cli->lastchar = *c;
            cli_send(cli, "\a", 1);
        }
        else if (cli->lastchar == CTRL('I'))
        {
            // double TAB
            cli_send(cli, "\r\n", 2);

            // show all completions
            // TODO
            return PROC_PRE_CONT;
        }

        return PROC_CONT;
    }

    if (*c == CTRL('P'))
    {
        cli->history_idx--;
        if (cli->history_idx < 0)
        {
            cli->history_idx = 0;
            cli_send(cli, "\a", 1);
        }
        else
        {
            char* newcmd = (char*)history_get(cli->history, cli->history_idx);
            _change_curr_cmd(cli, newcmd);
        }
        return PROC_CONT;
    }
    else if (*c == CTRL('N'))
    {
        cli->history_idx++;
        if (cli->history_idx >= history_num(cli->history))
        {
            cli->history_idx = history_num(cli->history) - 1;
            cli_send(cli, "\a", 1);
        }
        else
        {
            char* newcmd = (char*)history_get(cli->history, cli->history_idx);
            _change_curr_cmd(cli, newcmd);
        }
        return PROC_CONT;
    }

    if (*c == CTRL('B'))
    {
        // left , move cursor
        if (cli->cursor > 0)
        {
            cli_send(cli, "\b", 1);
            cli->cursor--;
        }
        return PROC_CONT;
    }

    if (*c == CTRL('F'))
    {
        // Right, move cursor
        if (cli->cursor < cli->len)
        {
            cli_send(cli, &cli->cmd[cli->cursor], 1);
            cli->cursor++;
        }
        return PROC_CONT;
    }

    if (*c == CTRL('A'))
    {
        // start of line
        if (cli->cursor > 0)
        {
            cli_send(cli, "\r", 1);
            cli_send(cli, cli->prompt, strlen(cli->prompt));
            cli->cursor = 0;
        }
        return PROC_CONT;
    }

    if (*c == CTRL('E'))
    {
        // end of line
        if (cli->cursor < cli->len)
        {
            cli_send(cli, &cli->cmd[cli->cursor], cli->len - cli->cursor);
            cli->cursor = cli->len;
        }
        return PROC_CONT;
    }

    return PROC_NONE;
}

static int _proc_login(struct cli* cli)
{
    if (strcmp(cli->cmd, cli->username) == 0)
    {
        if (cli->password)
        {
            cli->state = CLI_ST_PASSWORD;
            free(cli->prompt);
            cli->prompt = strdup("Password: ");
        }
        else
        {
            cli->state = CLI_ST_NORMAL;
            free(cli->prompt);
            cli->prompt = strdup("[taco]$ ");
            cli->count = 0;

            cli_send(cli, "\r\n", 2);
             _show_banner(cli);
       }

        return PROC_PRE_CONT;
    }

    cli_send(cli, "\n\r", 2);
    cli_send(cli, "Invalid Username.\n\r", strlen("Invalid Username.\n\r"));

    if (cli->count < CLI_RETRY_COUNT)
    {
        cli->count++;
        return PROC_PRE_CONT;
    }
    return PROC_ERROR;
}

static int _proc_password(struct cli* cli)
{
    if (strcmp(cli->cmd, cli->password) == 0)
    {
        cli->state = CLI_ST_NORMAL;
        free(cli->prompt);
        cli->prompt = strdup("[taco]$ ");
        cli->count = 0;

        cli_send(cli, "\r\n", 2);
        _show_banner(cli);
        return PROC_PRE_CONT;
    }

    cli_send(cli, "\n\r", 2);
    cli_send(cli, "Invalid Password.\n\r", strlen("Invalid Password.\n\r"));

    if (cli->count < CLI_RETRY_COUNT)
    {
        cli->count++;
        cli->state = CLI_ST_LOGIN;
        free(cli->prompt);
        cli->prompt = strdup("Username: ");

        return PROC_PRE_CONT;
    }
    return PROC_ERROR;
}

static int _is_empty_string(char* string)
{
    char* dup = strdup(string);
    char* scratch = NULL;
    char*  delimiters = " \r\n\t";
    char* text = strtok_r(dup, delimiters, &scratch);
    if (text == NULL)
    {
        free(dup);
        return 1;
    }
    free(dup);
    return 0;
}

int cli_process(struct cli* cli)
{
    CHECK_IF(cli == NULL, return CLI_FAIL, "cli is null");
    CHECK_IF(cli->is_init != 1, return CLI_FAIL, "cli is not init yet");
    CHECK_IF(cli->fd <= 0, return CLI_FAIL, "cli fd = %d invalid", cli->fd);

    int ret     = -1;
    int recvlen = -1;
    unsigned char c;

    recvlen = cli_recv(cli, &c, 1);
    if (recvlen <= 0)
    {
        // tcp recv failed or connection closed
        goto _ERROR;
    }

    ret = _proc_option(cli, c);
    if (ret == PROC_CONT)
    {
        goto _CONT;
    }

    ret = _proc_ctrl(cli, &c);
    if (ret == PROC_CONT)
    {
        goto _CONT;
    }
    else if (ret == PROC_ERROR)
    {
        goto _ERROR;
    }
    else if (ret == PROC_PRE_CONT)
    {
        goto _PRE_CONT;
    }

    if (c == '?' && cli->cursor == cli->len)
    {
        // show help str / description
        // TODO
        goto _CONT;
    }

    if (c == 0) goto _CONT;

    if (c == '\n')
    {
        goto _CONT;
    }

    if (c == '\r')
    {
        // 收到 return 除了換行以外，下次還要重新印 prompt
        if (cli->state == CLI_ST_NORMAL)
        {
            // 找適合的 command 並執行 callback
            // TODO

            if (!_is_empty_string(cli->cmd))
            {
                history_addstr(cli->history, cli->cmd);
            }
            cli->history_idx = history_num(cli->history);
        }
        else if (cli->state == CLI_ST_LOGIN)
        {
            ret = _proc_login(cli);
            if (ret == PROC_ERROR)
            {
                goto _ERROR;
            }
        }
        else if (cli->state == CLI_ST_PASSWORD)
        {
            ret = _proc_password(cli);
            if (ret == PROC_ERROR)
            {
                goto _ERROR;
            }
        }

        cli_send(cli, "\r\n", 2);
        goto _PRE_CONT;
    }

    // normal cahracter typed
    if (cli->len >= MAX_CLI_CMD_SIZE)
    {
        cli_send(cli, "\a", 1);
        goto _CONT;
    }

    if (cli->cursor == cli->len)
    {
        cli->cmd[cli->cursor] = c;
        cli->len++;
        cli->cursor++;
    }
    else
    {
        if (cli->insert_mode)
        {
            int i;
            for (i=cli->len; i>=cli->cursor; i--)
            {
                cli->cmd[i+1] = cli->cmd[i];
            }
            cli->cmd[cli->cursor] = c;
            cli_send(cli, &cli->cmd[cli->cursor], cli->len - cli->cursor + 1);

            for (i=0; i<(cli->len - cli->cursor + 1); i++)
            {
                cli_send(cli, "\b", 1);
            }
            cli->len++;
        }
        else
        {
            // replace mode
            cli->cmd[cli->cursor] = c;
        }
        cli->cursor++;
    }

    if (cli->state == CLI_ST_PASSWORD)
    {
        cli_send(cli, "*", 1);
    }
    else
    {
        cli_send(cli, &c, 1);
    }

    cli->oldcmd   = NULL;
    cli->oldlen   = 0;
    cli->lastchar = c;

_CONT:
    return CLI_OK;

_PRE_CONT:
    _pre_process(cli);
    return CLI_OK;

_ERROR:
    return CLI_FAIL;
}
