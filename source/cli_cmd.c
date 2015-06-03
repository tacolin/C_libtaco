#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

////////////////////////////////////////////////////////////////////////////////
static struct list _nodes   = {};
static cli_func _regular_fn = NULL;
////////////////////////////////////////////////////////////////////////////////

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

static struct array* _get_fit_cmds(struct list* list, char* str)
{
    struct array* ret = array_create(NULL);

    if ((list == NULL) || (str == NULL))
    {
        return ret;
    }

    struct cli_cmd* cmd = list_find(list, _compare_cmd_str, str);
    if (cmd)
    {
        array_add(ret, cmd);
    }
    else
    {
        void* obj;
        LIST_FOREACH(list, obj, cmd)
        {
            if ((cmd->type == CMD_TOKEN) && (cmd->str == strstr(cmd->str, str)))
            {
                array_add(ret, cmd);
            }
        }
    }
    return ret;
}

// static struct cli_cmd* _get_cmd(struct list* list, char* str)
// {
//     // 做完全的比對
//     struct cli_cmd* ret = list_find(list, _compare_cmd_str, str);
//     if (ret == NULL)
//     {
//         // 針對 token 做不完全的比對
//         struct cli_cmd* tmp;
//         void* obj;
//         int num = 0;
//         LIST_FOREACH(list, obj, tmp)
//         {
//             if ((tmp->type == CMD_TOKEN) && (tmp->str == strstr(tmp->str, str)))
//             {
//                 ret = tmp;
//                 num++;
//             }
//         }

//         // 如果比對到的只有一個，那就 ok
//         if (num != 1)
//         {
//             // 超過一個的話就當沒有比到
//             ret = NULL;
//         }
//     }
//     return ret;
// }

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

static int _compare_str(void* data, void* arg)
{
    struct cli_cmd* cmd = (struct cli_cmd*)data;
    char* str = (char*)arg;
    return (0 == strcmp(cmd->str, str)) ? 1 : 0;
}

struct cli_cmd* cli_install_cmd(struct cli_cmd* parent, char* string, cli_func func, int node_id, char* desc)
{
    CHECK_IF(string == NULL, return NULL, "string is null");

    struct cli_node* node = _get_node(node_id);
    CHECK_IF(node == NULL, return NULL, "node with id = %d is not in the list", node_id);

    char* pre_process_str = _pre_process_string(string);

    struct list* cmd_list = (parent) ? (&parent->sub_cmds) : (&node->cmds) ;
    struct cli_cmd* c = list_find(cmd_list, _compare_str, pre_process_str);
    if (c != NULL)
    {
        free(pre_process_str);
        return c;
    }

    c       = calloc(sizeof(*c), 1);
    c->str  = pre_process_str;
    c->type = _decide_cmd_type(c->str);
    c->desc = (desc) ? strdup(desc) : strdup(" ") ;
    c->func = func;
    list_init(&c->sub_cmds, _clean_cmd);

    list_append(cmd_list, c);

    if (c->type == CMD_INT)
    {
        sscanf(c->str, "<%ld-%ld>", &c->lbound, &c->ubound);
    }
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

static void _clean_str(void* data)
{
    if (data) free(data);
}

struct array* cli_get_completions(int node_id, char* string, char** output)
{
    CHECK_IF(string == NULL, return NULL, "string is null");
    CHECK_IF(strlen(string) > CMD_MAX_INPUT_SIZE, return NULL, "string len = %d invalid", strlen(string));

    struct cli_node* node = _get_node(node_id);
    CHECK_IF(node == NULL, return NULL, "node with id = %d is not in the list", node_id);

    struct list* cmd_list = &node->cmds;

    struct array* array = cli_string_to_array(string, " \r\t\n");
    CHECK_IF(array == NULL, return NULL, "cli_string_to_array failed");

    struct array* cmd_array = NULL;
    struct array* ret = NULL;

    *output = calloc(CMD_MAX_INPUT_SIZE, 1);
    *output[0] = '\0';

    int i;
    for (i=0; i<array->num; i++)
    {
        cmd_array = _get_fit_cmds(cmd_list, (char*)array->datas[i]);
        if (cmd_array->num == 1) // 只找到一個
        {
            struct cli_cmd* cmd = (struct cli_cmd*)(cmd_array->datas[0]);
            strcat(*output, cmd->str);
            cmd_list = &cmd->sub_cmds;
        }
        else if (cmd_array->num == 0) // 找不到
        {
            strcat(*output, (char*)array->datas[i]);
            cmd_list = NULL;
        }
        else // 找到很多個
        {
            if (i == array->num-1) // 是最後一次的比對嗎？
            {
                strcat(*output, (char*)array->datas[i]);
                break;
            }
            else // 視為找不到
            {
                strcat(*output, (char*)array->datas[i]);
                cmd_list = NULL;
            }
        }
        strcat(*output, " ");
        array_release(cmd_array);
        cmd_array = NULL;
    }
    cli_release_array(array);

    if (cmd_array)
    {
        ret = array_create(_clean_str);
        int i;
        struct cli_cmd* cmd;
        for (i=0; i<cmd_array->num; i++)
        {
            cmd = (struct cli_cmd*)cmd_array->datas[i];
            array_add(ret, strdup(cmd->str));
        }
        array_release(cmd_array);
    }
    return (ret == NULL) ? array_create(NULL) : ret ;
}

int cli_excute_cmd(int node_id, char* string)
{
    CHECK_IF(string == NULL, return CMD_FAIL, "string is null");
    CHECK_IF(strlen(string) > CMD_MAX_INPUT_SIZE, return CMD_FAIL, "string len = %d invalid", strlen(string));

    struct cli_node* node = _get_node(node_id);
    CHECK_IF(node == NULL, return CMD_FAIL, "node with id = %d is not in the list", node_id);

    struct list* cmd_list = &node->cmds;

    struct array* array = cli_string_to_array(string, " \r\t\n");
    CHECK_IF(array == NULL, return CMD_FAIL, "cli_string_to_array failed");

    struct cli_cmd* cmd = NULL;
    struct array* cmd_array = NULL;
    int i;
    for (i=0; i<array->num; i++)
    {
        cmd_array = _get_fit_cmds(cmd_list, (char*)array->datas[i]);
        if (cmd_array->num == 1) // 只有比到一個
        {
            cmd = cmd_array->datas[0];
            if (cmd->func && (i == array->num-1)) // 如果是最後一個  而且有 callback
            {
                cmd->func(cmd, NULL, 0, NULL);
                goto _END;
            }
            else
            {
                cmd_list = &cmd->sub_cmds; // 繼續比下一層
            }
        }
        else
        {
            if (_regular_fn) // 沒有比到 (比到多個算是沒比到啦)
            {
                _regular_fn(NULL, NULL, 0, NULL); // 執行 regular callback
            }
            goto _END;
        }
        array_release(cmd_array);
        cmd_array = NULL;
    }

_END:
    cli_release_array(array);
    if (cmd_array)
    {
        array_release(cmd_array);
    }
    return CMD_OK;
}

struct array* cli_string_to_array(char* string, char* delimiters)
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

void cli_release_array(struct array* array)
{
    if (array)
    {
        array_release(array);
    }
}
