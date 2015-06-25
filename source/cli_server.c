#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "cli_server.h"

////////////////////////////////////////////////////////////////////////////////

#define CTRL(key) (key - '@')

#define STR_EQUAL(str1, str2) (0 == strcmp(str1, str2))
#define IS_LAST_CMD(array, idx) (idx == array->num -1)
#define IS_OPT_CMD(str) (str[0] == '[')
#define IS_ALT_CMD(str) (str[0] == '(')

#define dprint(a, b...) fprintf(stdout, "%s(): "a"\n", __func__, ##b)
#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

////////////////////////////////////////////////////////////////////////////////

enum
{
    CLI_ST_LOGIN,
    CLI_ST_PASSWORD,
    CLI_ST_NORMAL,

    CLI_STATES
};

enum
{
    CMD_TOKEN,
    CMD_INT,
    CMD_IPV4,
    CMD_MACADDR,
    CMD_STRING,

    CMD_TYPES
};

enum
{
    PROC_CONT,
    PROC_PRE_CONT,
    PROC_ERROR,
    PROC_NONE,

    PROC_RETURN_VALUES
};

enum
{
    CLI_FULL_MATCH,
    CLI_PARTIAL_MATCH,
    CLI_MULTI_MATCHES,
    CLI_NO_MATCH,

    CLI_MATCH_TYPES
};

////////////////////////////////////////////////////////////////////////////////

struct cli_node
{
    int id;
    char* prompt;
    struct array* cmds;
};

struct cli_cmd
{
    int type;
    char* str;
    char* desc;
    cli_func func;

    struct array* sub_cmds;

    long ubound;
    long lbound;
};

////////////////////////////////////////////////////////////////////////////////

static int _compare_node_id(void* data, void* arg)
{
    struct cli_node* n = (struct cli_node*)data;
    int* id = (int*)arg;
    return (n->id == *id) ? 1 : 0 ;
}

static struct cli_node* _get_node(struct array* array, int id)
{
    return (struct cli_node*)array_find(array, _compare_node_id, &id);
}

static void _clean_node(void* data)
{
    if (data)
    {
        struct cli_node* n = (struct cli_node*)data;
        if (n->prompt) free(n->prompt);
        array_release(n->cmds);
        free(data);
    }
}

int cli_server_init(struct cli_server* server, char* local_ipv4, int local_port, int max_conn_num, char* username, char* password)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(max_conn_num <= 0, return CLI_FAIL, "server is null");

    memset(server, 0, sizeof(struct cli_server));

    int chk = tcp_server_init(&server->tcp_server, local_ipv4, local_port, max_conn_num);
    CHECK_IF(chk != TCP_OK, return CLI_FAIL, "tcp_server_init failed");

    server->fd    = server->tcp_server.fd;
    server->nodes = array_create(_clean_node);
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

    if (server->nodes)
    {
        array_release(server->nodes);
        server->nodes = NULL;
    }

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

int cli_server_banner(struct cli_server* server, char* banner)
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
    _show_prompt(cli);

    if (cli->oldcmd != NULL)
    {
        cli_send(cli, cli->oldcmd, cli->oldlen);

        cli->len    = cli->oldlen;
        cli->cursor = cli->oldlen;
        cli->pre    = 1;
        cli->oldcmd = NULL;
        cli->oldlen = 0;
    }
    else
    {
        memset(cli->cmd, 0, CLI_MAX_CMD_SIZE+1);
        cli->len    = 0;
        cli->cursor = 0;
    }

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

    cli->history     = history_create(CLI_MAX_CMD_SIZE+1, CLI_MAX_CMD_HISTORY_NUM);
    cli->pre         = 1;
    cli->insert_mode = 1;
    cli->fd          = cli->tcp.fd;
    cli->banner      = server->banner;
    cli->username    = server->username;
    cli->password    = server->password;
    cli->nodes       = server->nodes;
    cli->regular     = server->regular;
    cli->node_id     = server->default_node->id;
    cli->is_init     = 1;
    cli->state       = (cli->username == NULL) ? CLI_ST_NORMAL : CLI_ST_LOGIN ;

    if (cli->state == CLI_ST_NORMAL)
    {
        struct cli_node* node = _get_node(cli->nodes, cli->node_id);
        cli->prompt = strdup(node->prompt);
        _show_banner(cli);
        cli_send(cli, "\r\n", 2);
    }
    else if (cli->state == CLI_ST_LOGIN)
    {
        cli->prompt = strdup("UserName: ");
    }

    const char negotiate[] = "\xFF\xFB\x03" "\xFF\xFB\x01"
                             "\xFF\xFD\x03" "\xFF\xFD\x01";

    cli_send(cli, (void*)negotiate, strlen(negotiate)+1);
    _pre_process(cli);
    return CLI_OK;
}

static void _print_history(int idx, void* data)
{
    dprint("[%d] : %s", idx, (char*)data);
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

    cli->fd      = -1;
    cli->is_init = 0;
    return CLI_OK;
}

int cli_send(struct cli* cli, void* data, int data_len)
{
    CHECK_IF(cli == NULL, return -1, "cli is null");
    CHECK_IF(data_len <= 0, return -1, "data_len = %d invalid", data_len);
    return tcp_send(&cli->tcp, data, data_len);
}

static int _proc_option(struct cli* cli, unsigned char c)
{
    if ((c == 255) && (cli->is_option == 0))
    {
        cli->is_option++;
        return PROC_CONT;
    }

    if (cli->is_option)
    {
        if ((c >= 251) && (c <= 254))
        {
            cli->is_option = c;
            return PROC_CONT;
        }
        else if (c != 255)
        {
            cli->is_option = 0;
            return PROC_CONT;
        }

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
        int i;
        for (i=cli->cursor; i<=cli->len; i++)
        {
            cli->cmd[i] = cli->cmd[i+1];
        }

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
    int i;
    if (strlen(newcmd) > cli->len) // newcmd len > currcmd len
    {
        for (i=0; i<cli->len; i++)
        {
            if (cli->cmd[i] != newcmd[i])
            {
                break;
            }
        }

        int start = i;

        for (i=start; i<cli->len; i++)
        {
            cli_send(cli, "\b", 1);
        }

        for (i=start; i<strlen(newcmd); i++)
        {
            cli_send(cli, newcmd+i, 1);
        }

        snprintf(cli->cmd, CLI_MAX_CMD_SIZE, "%s", newcmd);

        cli->len = strlen(cli->cmd);
        cli->cursor = strlen(cli->cmd);
    }
    else // newcmd len <= currcmd len
    {
        for (i=strlen(newcmd); i<cli->len; i++)
        {
            cli_send(cli, "\b \b", 3);
        }

        for (i=0; i<strlen(newcmd); i++)
        {
            if (cli->cmd[i] != newcmd[i])
            {
                break;
            }
        }

        int start = i;

        for (i=start; i<cli->len-1; i++) // why cli->len-1 ? trial and error ...
        {
            cli_send(cli, "\b", 1);
        }

        for (i=start; i<strlen(newcmd); i++)
        {
            cli_send(cli, newcmd+i, 1);
        }

        snprintf(cli->cmd, CLI_MAX_CMD_SIZE, "%s", newcmd);

        cli->len = strlen(cli->cmd);
        cli->cursor = strlen(cli->cmd);
    }
    return;
}

static int _compare_cmd_str(void* data, void* arg)
{
    struct cli_cmd* c = (struct cli_cmd*)data;
    char* str = (char*)arg;
    bool ret;

    if (c->type == CMD_TOKEN)
    {
        ret = (0 == strcmp(c->str, str));
    }
    else if (c->type == CMD_INT)
    {
        // long lbound, ubound;
        // sscanf(c->str, "<%ld-%ld>", &lbound, &ubound);

        long value = strtol(str, NULL, 10);
        if ((value == ERANGE) || (value > c->ubound) || (value < c->lbound))
        {
            ret = false;
        }
        else
        {
            ret = true;
        }
    }
    else if (c->type == CMD_IPV4)
    {
        int ipv4[4];
        int num = sscanf(str, "%d.%d.%d.%d", &ipv4[0], &ipv4[1], &ipv4[2], &ipv4[3]);
        ret = (num == 4) ? true : false;
    }
    else if (c->type == CMD_MACADDR)
    {
        int mac[6];
        int num = sscanf(str, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        ret = (num == 6) ? true : false;
    }
    else if (c->type == CMD_STRING)
    {
        ret = (str != NULL) ? true : false ;
    }
    return ret;
}

static int _match_cmd(struct array* sub_cmds, char* cmd_str, struct array** matches)
{
    *matches = array_create(NULL);

    struct cli_cmd* cmd = (struct cli_cmd*)array_find(sub_cmds, _compare_cmd_str, cmd_str);
    if (cmd != NULL)
    {
        array_add(*matches, cmd);
        return CLI_FULL_MATCH;
    }
    else
    {
        int i;
        for (i=0; i<sub_cmds->num; i++)
        {
            cmd = (struct cli_cmd*)(sub_cmds->datas[i]);
            if ((cmd->type == CMD_TOKEN) && (cmd->str == strstr(cmd->str, cmd_str)))
            {
                array_add(*matches, cmd);
            }
        }

        if ((*matches)->num == 1)
        {
            return CLI_PARTIAL_MATCH;
        }
        else if ((*matches)->num > 1)
        {
            return CLI_MULTI_MATCHES;
        }
        else
        {
            return CLI_NO_MATCH;
        }
    }
}

static void _clean_str(void* data)
{
    if (data) free(data);
}

static struct array* _string_to_array(char* string, char* delimiters)
{
    CHECK_IF(string == NULL, return NULL, "string is null");
    CHECK_IF(delimiters == NULL, return NULL, "delimiters is null");

    struct array* a = array_create(_clean_str);

    char* scratch = NULL;
    char* dup     = strdup(string);
    char* text    = strtok_r(dup, delimiters, &scratch);
    while (text)
    {
        array_add(a, strdup(text));
        text = strtok_r(NULL, delimiters, &scratch);
    }
    free(dup);
    return a;
}

static void _show_desc(struct cli* cli)
{
    struct cli_node* node = _get_node(cli->nodes, cli->node_id);
    CHECK_IF(node == NULL, return, "_get_node failed by node_id = %d", cli->node_id);

    struct array* str_array = _string_to_array(cli->cmd, " \r\n\t");
    CHECK_IF(str_array == NULL, return, "_string_to_array failed");

    struct array* sub_cmds = node->cmds;
    int i;

    if (str_array->num == 0)
    {
        cli_send(cli, "\r\n", 2);

        for (i=0; i<sub_cmds->num; i++)
        {
            struct cli_cmd* cmd = (struct cli_cmd*)sub_cmds->datas[i];
            char* output_string;
            asprintf(&output_string, "%s\t: %s", cmd->str, cmd->desc);
            cli_send(cli, output_string, strlen(output_string)+1);
            cli_send(cli, "\r\n", 2);
            free(output_string);
        }
        cli_send(cli, "\r\n", 2);
    }
    else if ((cli->cmd[cli->len-1] == ' ') || (cli->cmd[cli->len-1] == '\t'))
    {
        for (i=0; i<str_array->num; i++)
        {
            struct cli_cmd* cmd = (struct cli_cmd*)array_find(sub_cmds, _compare_cmd_str, (char*)str_array->datas[i]);
            if (cmd == NULL)
            {
                goto _END;
            }
            sub_cmds = cmd->sub_cmds;
        }

        if (sub_cmds == NULL)
        {
            goto _END;
        }

        cli_send(cli, "\r\n", 2);

        for (i=0; i<sub_cmds->num; i++)
        {
            struct cli_cmd* cmd = (struct cli_cmd*)sub_cmds->datas[i];
            char* output_string;
            asprintf(&output_string, "%s\t: %s", cmd->str, cmd->desc);
            cli_send(cli, output_string, strlen(output_string)+1);
            cli_send(cli, "\r\n", 2);
            free(output_string);
        }
        cli_send(cli, "\r\n", 2);
    }
    else
    {
        for (i=0; i<str_array->num-1; i++)
        {
            struct cli_cmd* cmd = (struct cli_cmd*)array_find(sub_cmds, _compare_cmd_str, (char*)str_array->datas[i]);
            if (cmd == NULL)
            {
                goto _END;
            }
            sub_cmds = cmd->sub_cmds;
        }

        char* str = (char*)str_array->datas[str_array->num-1];

        if (sub_cmds == NULL)
        {
            goto _END;
        }

        cli_send(cli, "\r\n", 2);

        for (i=0; i<sub_cmds->num; i++)
        {
            struct cli_cmd* cmd = (struct cli_cmd*)sub_cmds->datas[i];
            if ((cmd->type == CMD_TOKEN) && (cmd->str == strstr(cmd->str, str)))
            {
                char* output_string;
                asprintf(&output_string, "%s\t: %s", cmd->str, cmd->desc);
                cli_send(cli, output_string, strlen(output_string)+1);
                cli_send(cli, "\r\n", 2);
                free(output_string);
            }
        }
        cli_send(cli, "\r\n", 2);
    }

    cli->oldcmd = cli->cmd;
    cli->oldlen = cli->len;

_END:
    array_release(str_array);
    return;
}

static int _proc_tab(struct cli* cli, unsigned char* c)
{
    if (cli->cursor != cli->len)
    {
        return PROC_CONT;
    }

    struct cli_node* node = _get_node(cli->nodes, cli->node_id);
    if (node == NULL)
    {
        return PROC_CONT;
    }

    char newcmd[CLI_MAX_CMD_SIZE+1] = {0};
    struct array* sub_cmds  = node->cmds;
    struct array* str_array = _string_to_array(cli->cmd, " \r\n\t");
    int ret = CLI_NO_MATCH;

    if (cli->lastchar == CTRL('I'))
    {
        // double TAB
        int i;
        for (i=0; i<str_array->num; i++)
        {
            struct cli_cmd* cmd = (struct cli_cmd*)array_find(sub_cmds, _compare_cmd_str, (char*)str_array->datas[i]);
            sub_cmds = cmd->sub_cmds;
        }

        for (i=0; i<sub_cmds->num; i++)
        {
            if ((i % 4) == 0)
            {
                cli_send(cli, "\r\n", 2);
            }

            struct cli_cmd* cmd = (struct cli_cmd*)sub_cmds->datas[i];
            cli_send(cli, cmd->str, strlen(cmd->str)+1);
            cli_send(cli, "\t", 1);
        }

        cli_send(cli, "\r\n", 2);
        cli_send(cli, "\r\n", 2);

        array_release(str_array);

        cli->lastchar = ' ';

        cli->oldcmd = cli->cmd;
        cli->oldlen = cli->len;

        return PROC_PRE_CONT;
    }
    else
    {
        // 1 TAB
        struct array* matches = NULL;
        int i;
        for (i=0; i<str_array->num; i++)
        {
            ret = _match_cmd(sub_cmds, (char*)str_array->datas[i], &matches);
            if (ret == CLI_FULL_MATCH)
            {
                struct cli_cmd* cmd = (struct cli_cmd*)matches->datas[0];
                array_release(matches);
                sub_cmds = cmd->sub_cmds;

                strcat(newcmd, cmd->str);
                strcat(newcmd, " ");
            }
            else if (ret == CLI_PARTIAL_MATCH)
            {
                struct cli_cmd* cmd = (struct cli_cmd*)matches->datas[0];
                array_release(matches);
                sub_cmds = cmd->sub_cmds;

                strcat(newcmd, cmd->str);
                strcat(newcmd, " ");
            }
            else if (ret == CLI_MULTI_MATCHES)
            {
                strcat(newcmd, (char*)str_array->datas[i]);
                array_release(matches);

                cli->lastchar = CTRL('I');
                break;
            }
            else //(ret == CLI_NO_MATCH)
            {
                strcat(newcmd, (char*)str_array->datas[i]);
                array_release(matches);

                cli->lastchar = CTRL('I');
                break;
            }
        }

        if ((cli->cmd[cli->len-1] == ' ') || (cli->cmd[cli->len-1] == '\t') || (cli->len == 0))
        {
            if (sub_cmds->num == 1)
            {
                struct cli_cmd* cmd = (struct cli_cmd*)sub_cmds->datas[0];
                if (cmd->type == CMD_TOKEN)
                {
                    strcat(newcmd, cmd->str);
                    strcat(newcmd, " ");
                }
                else
                {
                    cli->lastchar = CTRL('I');
                }
            }
            else
            {
                cli->lastchar = CTRL('I');
            }
        }

        array_release(str_array);
        _change_curr_cmd(cli, newcmd);

        cli->oldlen = cli->len;
        return PROC_CONT;
    }
}

static int _proc_ctrl(struct cli* cli, unsigned char *c)
{
    if (*c == 27)
    {
        cli->esc = 1;
        return PROC_CONT;
    }

    if (cli->esc)
    {
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
        else
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
        return _proc_tab(cli, c);
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
            struct cli_node* node = _get_node(cli->nodes, cli->node_id);
            cli->prompt = strdup(node->prompt);
            cli->count = 0;

            cli_send(cli, "\r\n", 2);
             _show_banner(cli);
       }

        return PROC_PRE_CONT;
    }

    cli_send(cli, "\n\r", 2);
    cli_send(cli, "Invalid Username.\n\r", strlen("Invalid Username.\n\r"));

    if (cli->count < CLI_MAX_RETRY_NUM)
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

        struct cli_node* node = _get_node(cli->nodes, cli->node_id);
        cli->prompt = strdup(node->prompt);
        cli->count = 0;

        cli_send(cli, "\r\n", 2);
        _show_banner(cli);
        return PROC_PRE_CONT;
    }

    cli_send(cli, "\n\r", 2);
    cli_send(cli, "Invalid Password.\n\r", strlen("Invalid Password.\n\r"));

    if (cli->count < CLI_MAX_RETRY_NUM)
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

static struct array* _get_fit_cmds(struct array* source, char* str)
{
    struct array* ret = array_create(NULL);

    if ((source == NULL) || (str == NULL)) return ret;

    struct cli_cmd* cmd = (struct cli_cmd*)array_find(source, _compare_cmd_str, str);
    if (cmd)
    {
        array_add(ret, cmd);
    }
    else
    {
        int i;
        for (i=0; i<source->num; i++)
        {
            cmd = (struct cli_cmd*)source->datas[i];
            if ((cmd->type == CMD_TOKEN) && (cmd->str == strstr(cmd->str, str)))
            {
                array_add(ret, cmd);
            }
        }
    }
    return ret;
}

static int _execute_cmd(struct cli* cli, int node_id, char* string)
{
    CHECK_IF(cli == NULL, return CLI_FAIL, "cli is null");
    CHECK_IF(cli->is_init != 1, return CLI_FAIL, "cli is not init yet");
    CHECK_IF(cli->fd <= 0, return CLI_FAIL, "cli fd = %d invalid", cli->fd);

    CHECK_IF(string == NULL, return CLI_FAIL, "string is null");
    CHECK_IF(strlen(string) > CLI_MAX_CMD_SIZE, return CLI_FAIL, "string len = %zd invalid", strlen(string));

    struct cli_node* node = _get_node(cli->nodes, node_id);
    CHECK_IF(node == NULL, return CLI_FAIL, "node with id = %d does not exist", node_id);

    struct array* str_array = _string_to_array(string, " \r\t\n");
    CHECK_IF(str_array == NULL, return CLI_FAIL, "_string_to_array failed");

    struct array* cmds      = node->cmds;
    struct array* fit_cmds  = NULL;
    struct array* arguments = array_create(NULL);
    int i;
    bool running = true;
    for (i=0; i<str_array->num && running; i++)
    {
        fit_cmds = _get_fit_cmds(cmds, (char*)str_array->datas[i]);
        if (fit_cmds->num == 1)
        {
            struct cli_cmd* c = (struct cli_cmd*)fit_cmds->datas[0];
            if (c->type != CMD_TOKEN)
            {
                array_add(arguments, str_array->datas[i]);
            }

            if (i == str_array->num-1)
            {
                if (c->func)
                {
                    cli_send(cli, "\r\n", 2);
                    c->func(cli, arguments->num, (char**)arguments->datas);
                    running = false;
                }
                else
                {
                    if (cli->regular)
                    {
                        cli_send(cli, "\r\n", 2);
                        cli->regular(cli, 1, &string);
                    }
                }
            }
            cmds = c->sub_cmds;
        }
        else
        {
            if (cli->regular)
            {
                cli_send(cli, "\r\n", 2);
                cli->regular(cli, 1, &string);
            }

            running = false;
        }
        array_release(fit_cmds);
    }
    array_release(str_array);
    array_release(arguments);
    return CLI_OK;
}

int cli_execute_cmd(struct cli* cli, char* string)
{
    return _execute_cmd(cli, cli->node_id, string);
}

static int _process(struct cli* cli, char c)
{
    int ret = -1;

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
        _show_desc(cli); // show help str / description
        goto _PRE_CONT;
    }

    if (c == 0) goto _CONT;

    if (c == '\n')
    {
        goto _CONT;
    }

    if (c == '\r')
    {
        if (cli->state == CLI_ST_NORMAL)
        {
            _execute_cmd(cli, cli->node_id, cli->cmd);

            if (!_is_empty_string(cli->cmd))
            {
                history_addstr(cli->history, cli->cmd);
                dprint("save history : %s", cli->cmd);
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
    if (cli->len >= CLI_MAX_CMD_SIZE)
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

int cli_process(struct cli* cli)
{
    CHECK_IF(cli == NULL, return CLI_FAIL, "cli is null");
    CHECK_IF(cli->is_init != 1, return CLI_FAIL, "cli is not init yet");
    CHECK_IF(cli->fd <= 0, return CLI_FAIL, "cli fd = %d invalid", cli->fd);

    unsigned char buf[CLI_MAX_CMD_SIZE+1];

    int recvlen = tcp_recv(&cli->tcp, buf, CLI_MAX_CMD_SIZE+1);
    CHECK_IF(recvlen <= 0, return CLI_FAIL, "tcp_recv failed");

    dprint("recvlen = %d", recvlen);

    int i;
    int ret;
    for (i=0; i<recvlen; i++)
    {
        ret = _process(cli, buf[i]);
        CHECK_IF(ret != CLI_OK, return ret, "_process failed");
    }

    return CLI_OK;
}

int cli_server_regular_func(struct cli_server* server, cli_func regular)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(server->fd <= 0, return CLI_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(server->is_init != 1, return CLI_FAIL, "server is not init yet");
    server->regular = regular;
    return CLI_OK;
}

static void _clean_cmd(void* data)
{
    if (data)
    {
        struct cli_cmd* c = (struct cli_cmd*)data;
        if (c->str) free(c->str);
        if (c->desc) free(c->desc);
        array_release(c->sub_cmds);
        free(data);
    }
}

int cli_server_install_node(struct cli_server* server, int id, char* prompt)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(server->fd <= 0, return CLI_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(server->is_init != 1, return CLI_FAIL, "server is not init yet");
    CHECK_IF(prompt == NULL, return CLI_FAIL, "prompt is null");

    struct cli_node* node = _get_node(server->nodes, id);
    CHECK_IF(node != NULL, return CLI_FAIL, "node with id = %d has already exists", id);

    node         = calloc(sizeof(*node), 1);
    node->id     = id;
    node->prompt = strdup(prompt);
    node->cmds   = array_create(_clean_cmd);

    array_add(server->nodes, node);

    if (server->default_node == NULL) server->default_node = node;

    return CLI_OK;
}

static char* _pre_process_string(char* string)
{
    char* tmp = strdup(string);
    int len = strlen(tmp);

    int i;
    for (i=0; i<strlen(tmp); i++)
    {
        if (tmp[i] == '(')
        {
            int count = 1;
            int begin = i;
            int end;
            for (end=begin+1; end<strlen(tmp); end++)
            {
                if (tmp[end] == '(') count++;

                if (tmp[end] == ')') count--;

                if (count == 0)
                {
                    int j;
                    for (j=begin+1; j<=end; j++)
                    {
                        while (tmp[j] == ' ')
                        {
                            int k;
                            for (k=j; k<strlen(tmp)-1; k++)
                            {
                                tmp[k] = tmp[k+1];
                            }
                            end--;
                            len--;
                        }
                    }
                    break;
                }
            }
        }
        else if (tmp[i] == '[')
        {
            int count = 1;
            int begin = i;
            int end;
            for (end=begin+1; end<strlen(tmp); end++)
            {
                if (tmp[end] == '[') count++;

                if (tmp[end] == ']') count--;

                if (count == 0)
                {
                    int j;
                    for (j=begin+1; j<=end; j++)
                    {
                        while (tmp[j] == ' ')
                        {
                            int k;
                            for (k=j; k<strlen(tmp)-1; k++)
                            {
                                tmp[k] = tmp[k+1];
                            }
                            end--;
                            len--;
                        }
                    }
                    break;
                }
            }
        }
        else if (tmp[i] == '<')
        {
            int count = 1;
            int begin = i;
            int end;
            for (end=begin+1; end<strlen(tmp); end++)
            {
                if (tmp[end] == '<') count++;

                if (tmp[end] == '>') count--;

                if (count == 0)
                {
                    int j;
                    for (j=begin+1; j<=end; j++)
                    {
                        while (tmp[j] == ' ')
                        {
                            int k;
                            for (k=j; k<strlen(tmp)-1; k++)
                            {
                                tmp[k] = tmp[k+1];
                            }
                            end--;
                            len--;
                        }
                    }
                    break;
                }
            }
        }
    }
    tmp[len] = '\0';
    return tmp;
}

static bool _is_all_upper(char* text)
{
    int i;
    for (i=0; i<strlen(text)-1; i++)
    {
        if (islower((int)text[i])) return false;
    }
    return true;
}

static int _decide_cmd_type(char* text)
{
    if (text[0] == '<') // <1-100>
    {
        return CMD_INT;
    }
    else if (text[0] == 'A') // A.B.C.D/M
    {
        return CMD_IPV4;
    }
    else if (text[0] == 'X') // X:X:X:X:X:X
    {
        return CMD_MACADDR;
    }
    else if (_is_all_upper(text))
    {
        return CMD_STRING;
    }
    return CMD_TOKEN;
}

static int _compare_str(void* data, void* arg)
{
    struct cli_cmd* cmd = (struct cli_cmd*)data;
    char* str = (char*)arg;
    return (0 == strcmp(cmd->str, str)) ? 1 : 0;
}

static struct cli_cmd* _install_cmd_element(struct cli_server* server, struct cli_cmd* parent, char* string, cli_func func, int node_id, char* desc)
{
    CHECK_IF(string == NULL, return NULL, "string is null");

    struct cli_node* node = _get_node(server->nodes, node_id);
    CHECK_IF(node == NULL, return NULL, "node with id = %d is not in the list", node_id);

    char* pre_process_str = _pre_process_string(string);
    struct array* cmds = (parent) ? (parent->sub_cmds) : (node->cmds);
    struct cli_cmd* c = (struct cli_cmd*)array_find(cmds, _compare_str, pre_process_str);
    if (c != NULL)
    {
        free(pre_process_str);
        c->func = func;
        return c;
    }

    int type    = _decide_cmd_type(pre_process_str);
    long ubound = 0;
    long lbound = 0;
    if (type == CMD_INT)
    {
        if (2 != sscanf(pre_process_str, "<%ld-%ld>", &lbound, &ubound))
        {
            derror("parse %s failed", pre_process_str);
            free(pre_process_str);
            return NULL;
        }
    }

    c         = calloc(sizeof(*c), 1);
    c->str    = pre_process_str;
    c->type   = type;
    c->desc   = (desc) ? strdup(desc) : strdup(" ") ;
    c->func   = func;
    c->lbound = lbound;
    c->ubound = ubound;
    c->sub_cmds = array_create(_clean_cmd);

    array_add(cmds, c);
    return c;
}

static bool _is_alt_cmd_with_opt(char* str)
{
    int i;
    for (i=0; i<strlen(str)-1; i++)
    {
        if ((str[i] == '(' && str[i+1] == '|')
            || (str[i] == '|' && str[i+1] == '|')
            || (str[i] == '|' && str[i+1] == ')'))
        {
            // (|add|del) 會被視為是 [(add|del)]
            return true;
        }
    }
    return false;
}

static int _install_wrapper(struct cli_server* server, struct cli_cmd* parent, struct array* str_array, cli_func func, int node_id, struct array* desc_array, int idx)
{
    CHECK_IF(str_array == NULL, return CLI_FAIL, "str_array is null");
    CHECK_IF(idx < 0, return CLI_FAIL, "idx = %d invalid", idx);

    char* str  = (char*)str_array->datas[idx];
    char* desc = (idx > desc_array->num) ? NULL : (char*)desc_array->datas[idx];

    if (IS_LAST_CMD(str_array, idx))
    {
        if (IS_OPT_CMD(str))
        {
            char* content = NULL;
            sscanf(str, "[%s]", content);

            _install_cmd_element(server, parent, content, func, node_id, desc);
            parent->func = func;
        }
        else if (IS_ALT_CMD(str))
        {
            struct array* content_array = _string_to_array(str, "(|)");
            char* content;
            int i;
            for (i=0; i<content_array->num; i++)
            {
                content = (char*)content_array->datas[i];
                _install_cmd_element(server, parent, content, func, node_id, desc);
            }
            array_release(content_array);

            if (_is_alt_cmd_with_opt(str))
            {
                parent->func = func;
            }
        }
        else
        {
            _install_cmd_element(server, parent, str, func, node_id, desc);
        }
    }
    else
    {
        struct cli_cmd* c = NULL;
        if (IS_OPT_CMD(str))
        {
            char* content = NULL;
            sscanf(str, "[%s]", content);

            c = _install_cmd_element(server, parent, content, NULL, node_id, desc);
            _install_wrapper(server, c, str_array, func, node_id, desc_array, idx+1);
            _install_wrapper(server, parent, str_array, func, node_id, desc_array, idx+1);
        }
        else if (IS_ALT_CMD(str))
        {
            struct array* content_array = _string_to_array(str, "(|)");
            char* content;
            int i;
            for (i=0; i<content_array->num; i++)
            {
                content = (char*)content_array->datas[i];
                c = _install_cmd_element(server, parent, content, NULL, node_id, desc);
                _install_wrapper(server, c, str_array, func, node_id, desc_array, idx+1);
            }
            array_release(content_array);

            if (_is_alt_cmd_with_opt(str))
            {
                _install_wrapper(server, parent, str_array, func, node_id, desc_array, idx+1);
            }
        }
        else
        {
            c = _install_cmd_element(server, parent, str, NULL, node_id, desc);
            _install_wrapper(server, c, str_array, func, node_id, desc_array, idx+1);
        }
    }
    return CLI_OK;
}

int cli_server_install_cmd(struct cli_server* server, int node_id, struct cli_cmd_cfg* cfg)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(server->fd <= 0, return CLI_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(server->is_init != 1, return CLI_FAIL, "server is not init yet");

    CHECK_IF(cfg == NULL, return CLI_FAIL, "cfg is null");
    CHECK_IF(cfg->func == NULL, return CLI_FAIL, "cfg->func is null");
    CHECK_IF(cfg->cmd_str == NULL, return CLI_FAIL, "cfg->cmd_str is null");

    // 先把字串處理過
    char* dup = _pre_process_string(cfg->cmd_str);

    // 把字串切成 array
    struct array* str_array = _string_to_array(dup, " \r\n\t");

    // 把 descrption 也切成 array
    struct array* desc_array = array_create(NULL);
    int i;
    for (i=0; cfg->descs[i] != NULL; i++)
    {
        array_add(desc_array, cfg->descs[i]);
    }

    // 開始 install cmd
    _install_wrapper(server, NULL, str_array, cfg->func, node_id, desc_array, 0);

    array_release(desc_array);
    array_release(str_array);
    free(dup);
    return CLI_OK;
}

int cli_server_default_node(struct cli_server* server, int id)
{
    CHECK_IF(server == NULL, return CLI_FAIL, "server is null");
    CHECK_IF(server->fd <= 0, return CLI_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(server->is_init != 1, return CLI_FAIL, "server is not init yet");

    struct cli_node* node = _get_node(server->nodes, id);
    CHECK_IF(node != NULL, return CLI_FAIL, "node with id = %d has already exists", id);

    server->default_node = node;
    return CLI_OK;
}

int cli_change_node(struct cli* cli, int node_id)
{
    CHECK_IF(cli == NULL, return CLI_FAIL, "cli is null");
    CHECK_IF(cli->is_init != 1, return CLI_FAIL, "cli is not init yet");
    CHECK_IF(cli->fd <= 0, return CLI_FAIL, "cli fd = %d invalid", cli->fd);

    struct cli_node* node = _get_node(cli->nodes, node_id);
    CHECK_IF(node == NULL, return CLI_FAIL, "find no node with id = %d", node_id);

    cli->node_id = node_id;
    free(cli->prompt);
    cli->prompt = strdup(node->prompt);

    return CLI_OK;
}

