#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_cmd.h"

#define STR_EQUAL(str1, str2) (0 == strcmp(str1, str2))

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define dprint(a, b...) fprintf(stdout, "%s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static struct list _nodes   = {};
static cli_func _regular_fn = NULL;

static bool _is_all_upper(char* text)
{
    int i;
    for (i=0; i<strlen(text)-1; i++)
    {
        if (islower((int)text[i])) return false;
    }
    return true;
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

// static int _calc_desc_num(char** descs)
// {
//     CHECK_IF(descs == NULL, return -1, "descs is null");
//     int i;
//     for (i=0; descs[i]; i++);
//     return i;
// }

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

static int _compare_cmd_str(void* data, void* arg)
{
    struct cli_cmd* c = (struct cli_cmd*)data;
    char* str = (char*)arg;
    return (0 == strcmp(c->str, str)) ? 1 : 0 ;
}

static struct cli_cmd* _get_cmd(struct list* list, char* str)
{
    return list_find(list, _compare_cmd_str, str);
}

static int _compare_node_id(void* data, void* arg)
{
    struct cli_node* n = (struct cli_node*)data;
    int* id = (int*)arg;
    return (n->id == *id) ? 1 : 0 ;
}

static void _clean_node(void* data)
{
    if (data)
    {
        struct cli_node* n = (struct cli_node*)data;
        if (n->prompt) free(n->prompt);
        list_clean(&n->cmds);
        free(data);
    }
}

static struct cli_node* _get_node(int id)
{
    return list_find(&_nodes, _compare_node_id, &id);
}

static void _clean_cmd(void* data)
{
    if (data)
    {
        struct cli_cmd* c = (struct cli_cmd*)data;
        if (c->str) free(c->str);
        if (c->desc) free(c->desc);
        list_clean(&c->sub_cmds);
        free(data);
    }
}

struct cli_cmd* cli_install_cmd(struct cli_cmd* parent, char* string, cli_func func, int node_id, char* desc)
{
    CHECK_IF(string == NULL, return NULL, "string is null");

    struct cli_node* node = _get_node(node_id);
    CHECK_IF(node == NULL, return NULL, "node with id = %d is not in the list", node_id);

    struct list* cmd_list = (parent) ? (&parent->sub_cmds) : (&node->cmds) ;
    struct cli_cmd* c = _get_cmd(cmd_list, string);
    CHECK_IF(c != NULL, return c, "cmd (%s) has already existed", string);

    c       = calloc(sizeof(*c), 1);
    c->str  = _pre_process_string(string);
    c->type = _decide_cmd_type(c->str);
    c->desc = (desc) ? strdup(desc) : strdup(" ") ;
    c->func = func;
    list_init(&c->sub_cmds, _clean_cmd);

    list_append(cmd_list, c);
    return c;
}

struct cli_node* cli_install_node(int id, char* prompt)
{
    CHECK_IF(prompt == NULL, return NULL, "prompt is null");

    struct cli_node* node = _get_node(id);
    CHECK_IF(node != NULL, return node, "id = %d is already in the list", id);

    node = calloc(sizeof(*node), 1);
    node->id = id;
    node->prompt = strdup(prompt);
    list_init(&node->cmds, _clean_cmd);

    list_append(&_nodes, node);
    return node;
}

int cli_sys_init(void)
{
    list_init(&_nodes, _clean_node);
    return CMD_OK;
}

void cli_sys_uninit(void)
{
    list_clean(&_nodes);
}

void cli_install_regular(cli_func func)
{
    _regular_fn = func;
}

void _show_cmd(struct cli_cmd* cmd, int layer)
{
    static char* spaces = "                                                     ";

    CHECK_IF(cmd == NULL, return, "cmd is null");

    dprint("%.*s %s (%s)", layer*2, spaces, cmd->str, cmd->desc);

    void* obj;
    struct cli_cmd* sub;
    LIST_FOREACH(&cmd->sub_cmds, obj, sub)
    {
        _show_cmd(sub, layer+1);
    }
    return;
}

void cli_show_cmds(int node_id)
{
    struct cli_node* node = _get_node(node_id);
    CHECK_IF(node == NULL, return, "node with id = %d is not in the list", node_id);

    void* obj;
    struct cli_cmd* cmd;
    LIST_FOREACH(&node->cmds, obj, cmd)
    {
        _show_cmd(cmd, 1);
    }
    return;
}

int cli_compare_string(int node_id, char* string)
{
    CHECK_IF(string == NULL, return CMD_FAIL, "string is null");

    struct cli_node* node = _get_node(node_id);
    CHECK_IF(node == NULL, return CMD_FAIL, "node with id = %d is not in the list", node_id);

    struct list* cmd_list = &node->cmds;

    char* dup = strdup(string);
    char* scratch = NULL;
    char* delimiters = " \r\t\n";
    char* text = strtok_r(dup, delimiters, &scratch);
    while (text)
    {
        struct cli_cmd* cmd = _get_cmd(cmd_list, text);
        if (cmd == NULL)
        {
            if (_regular_fn) _regular_fn(cmd, NULL, 0, NULL);
            break;
        }

        if (cmd->func)
        {
            cmd->func(cmd, NULL, 0, NULL);
            break;
        }
        cmd_list = &(cmd->sub_cmds);
        text = strtok_r(NULL, delimiters, &scratch);
    }
    free(dup);
    return CMD_OK;
}
