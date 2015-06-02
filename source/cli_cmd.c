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

static struct list _node_list = {};

static int _calc_desc_num(char** descs)
{
    CHECK_IF(descs == NULL, return -1, "descs is null");
    int i;
    for (i=0; descs[i]; i++);
    return i;
}

static int _split_cmd_str_to_elem(struct cli_cmd* cmd, char** descs)
{
    CHECK_IF(cmd == NULL, return CMD_FAIL, "cmd is null");
    CHECK_IF(cmd->cmd_str == NULL, return CMD_FAIL, "cmd->cmd_str is null");
    CHECK_IF(descs == NULL, return CMD_FAIL, "descs is null");

    int desc_num     = _calc_desc_num(descs);
    int desc_idx     = 0;
    char* dup        = strdup(cmd->cmd_str);
    char* scratch    = NULL;
    char* delimeters = " \r\n\t";
    char* text       = strtok_r(dup, delimeters, &scratch);
    while (text)
    {
        struct cli_elem* e = calloc(sizeof(*e), 1);
        e->elem_str = strdup(text);
        e->desc     = (desc_idx < desc_num) ? strdup(descs[desc_idx]) : strdup(" ") ;
        desc_idx++;

        if (text[0] == '(')
        {
            e->is_alternative = true;
        }
        else if (text[0] == '[')
        {
            e->is_option = true;
        }
        list_append(&cmd->elem_list, e);

        text = strtok_r(NULL, delimeters, &scratch);
    }
    free(dup);
    return CMD_OK;
}

static int _find_node(void* data, void* arg)
{
    struct cli_node* n = (struct cli_node*)data;
    return STR_EQUAL(n->name, (char*)arg) ? 1 : 0;
}

void cli_show_cmds(char* node_name)
{
    CHECK_IF(node_name == NULL, return, "node_name is null");

    struct cli_node* node = list_find(&_node_list, _find_node, node_name);
    CHECK_IF(node == NULL, return, "node (name = %s) is not in the list", node_name);

    int idx = 0;
    void* obj;
    struct cli_cmd* cmd;
    LIST_FOREACH(&node->cmd_list, obj, cmd)
    {
        dprint("cmd[%d] = %s", idx, cmd->cmd_str);

        void* elem_obj;
        struct cli_elem* elem;
        int elem_idx = 0;
        LIST_FOREACH(&cmd->elem_list, elem_obj, elem)
        {
            dprint("\telem[%d] = %s, desc = %s", elem_idx, elem->elem_str, elem->desc);
            elem_idx++;
        }
        idx++;
    }
    return;
}

static void _clean_elem(void* input)
{
    if (input)
    {
        struct cli_elem* e = (struct cli_elem*)input;
        free(e->elem_str);
        free(e->desc);
        free(e);
    }
}

static void _clean_cmd(void* input)
{
    if (input)
    {
        struct cli_cmd* c = (struct cli_cmd*)input;
        free(c->cmd_str);
        list_clean(&c->elem_list);
        free(c);
    }
}

int cli_install_cmd(char* node_name, struct cli_cmd_cfg* cfg)
{
    CHECK_IF(node_name == NULL, return CMD_FAIL, "node_name is null");
    CHECK_IF(cfg == NULL, return CMD_FAIL, "cfg is null");
    return cli_install_cmd_ex(node_name, cfg->cmd_str, cfg->func, cfg->descs);
}

int cli_install_cmd_ex(char* node_name, char* string, cli_func func, char** descs)
{
    CHECK_IF(node_name == NULL, return CMD_FAIL, "node_name is null");
    CHECK_IF(string == NULL, return CMD_FAIL, "string is null");
    CHECK_IF(func == NULL, return CMD_FAIL, "func is null");
    CHECK_IF(descs == NULL, return CMD_FAIL, "descs is null");

    struct cli_node* node = list_find(&_node_list, _find_node, node_name);
    CHECK_IF(node == NULL, return CMD_FAIL, "node (name = %s) is not in the list", node_name);

    struct cli_cmd* c = calloc(sizeof(*c), 1);
    c->cmd_str        = _pre_process_string(string);
    c->func           = func;
    list_init(&c->elem_list, _clean_elem);

    int chk = _split_cmd_str_to_elem(c, descs);
    CHECK_IF(chk != CMD_OK, goto _ERROR, "_split_cmd_str_to_elem failed");

    list_append(&node->cmd_list, c);
    return CMD_OK;

_ERROR:
    _clean_cmd(c);
    return CMD_FAIL;
}

int cli_install_node(char* name, char* prompt)
{
    CHECK_IF(name == NULL, return CMD_FAIL, "name is null");
    CHECK_IF(prompt == NULL, return CMD_FAIL, "prompt is null");

    struct cli_node* n = list_find(&_node_list, _find_node, name);
    CHECK_IF(n != NULL, return CMD_FAIL, "node (name = %s) is already in the list", name);

    n = calloc(sizeof(*n), 1);
    n->name = strdup(name);
    n->prompt = strdup(prompt);
    list_init(&n->cmd_list, _clean_cmd);

    list_append(&_node_list, n);
    return CMD_OK;
}

static void _clean_node(void* input)
{
    if (input)
    {
        struct cli_node* n = (struct cli_node*)input;
        free(n->name);
        free(n->prompt);
        list_clean(&n->cmd_list);
        free(n);
    }
}

int cli_sys_init(void)
{
    list_init(&_node_list, _clean_node);
    return CMD_OK;
}

void cli_sys_uninit(void)
{
    list_clean(&_node_list);
}

int cli_set_default_node(char* node_name)
{
    CHECK_IF(node_name == NULL, return CMD_FAIL, "node_name is null");

    struct cli_node* node = list_find(&_node_list, _find_node, node_name);
    CHECK_IF(node == NULL, return CMD_FAIL, "node (name = %s) is not in the list", node_name);

    list_remove(&_node_list, node);
    list_insert(&_node_list, node);
    return CMD_OK;
}
