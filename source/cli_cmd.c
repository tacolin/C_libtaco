// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <errno.h>

// #include "cli_cmd.h"

// #define STR_EQUAL(str1, str2) (0 == strcmp(str1, str2))
// #define IS_LAST_CMD(array, idx) (idx == array->num -1)
// #define IS_OPT_CMD(str) (str[0] == '[')
// #define IS_ALT_CMD(str) (str[0] == '(')

// #define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
// #define dprint(a, b...) fprintf(stdout, "%s(): "a"\n", __func__, ##b)
// #define CHECK_IF(assertion, error_action, ...) \
// {\
//     if (assertion) \
//     { \
//         derror(__VA_ARGS__); \
//         {error_action;} \
//     }\
// }

// struct cli_node
// {
//     int id;
//     char* prompt;
//     struct array* cmds;
// };

// struct cli_cmd
// {
//     int type;
//     char* str;
//     char* desc;
//     cli_func func;

//     struct array* sub_cmds;

//     long ubound;
//     long lbound;
// };

// enum
// {
//     CMD_TOKEN,
//     CMD_INT,
//     CMD_IPV4,
//     CMD_MACADDR,
//     CMD_STRING,

//     CMD_TYPES
// };

// ////////////////////////////////////////////////////////////////////////////////
// static struct array* _nodes       = NULL;
// static cli_func _regular_fn       = NULL;
// ////////////////////////////////////////////////////////////////////////////////

// static void _clean_str(void* data)
// {
//     if (data) free(data);
// }

// static bool _is_alt_cmd_with_opt(char* str)
// {
//     int i;
//     for (i=0; i<strlen(str)-1; i++)
//     {
//         if ((str[i] == '(' && str[i+1] == '|')
//             || (str[i] == '|' && str[i+1] == '|')
//             || (str[i] == '|' && str[i+1] == ')'))
//         {
//             // (|add|del) 會被視為是 [(add|del)]
//             return true;
//         }
//     }
//     return false;
// }

// static struct array* _string_to_array(char* string, char* delimiters)
// {
//     CHECK_IF(string == NULL, return NULL, "string is null");
//     CHECK_IF(delimiters == NULL, return NULL, "delimiters is null");

//     struct array* a = array_create(_clean_str);

//     char* scratch = NULL;
//     char* dup     = strdup(string);
//     char* text    = strtok_r(dup, delimiters, &scratch);
//     while (text)
//     {
//         array_add(a, strdup(text));
//         text = strtok_r(NULL, delimiters, &scratch);
//     }
//     free(dup);
//     return a;
// }

// static bool _is_all_upper(char* text)
// {
//     int i;
//     for (i=0; i<strlen(text)-1; i++)
//     {
//         if (islower((int)text[i])) return false;
//     }
//     return true;
// }

// static char* _pre_process_string(char* string)
// {
//     char* tmp = strdup(string);
//     int len = strlen(tmp);

//     int i;
//     for (i=0; i<strlen(tmp); i++)
//     {
//         if (tmp[i] == '(')
//         {
//             int count = 1;
//             int begin = i;
//             int end;
//             for (end=begin+1; end<strlen(tmp); end++)
//             {
//                 if (tmp[end] == '(') count++;

//                 if (tmp[end] == ')') count--;

//                 if (count == 0)
//                 {
//                     int j;
//                     for (j=begin+1; j<=end; j++)
//                     {
//                         while (tmp[j] == ' ')
//                         {
//                             int k;
//                             for (k=j; k<strlen(tmp)-1; k++)
//                             {
//                                 tmp[k] = tmp[k+1];
//                             }
//                             end--;
//                             len--;
//                         }
//                     }
//                     break;
//                 }
//             }
//         }
//         else if (tmp[i] == '[')
//         {
//             int count = 1;
//             int begin = i;
//             int end;
//             for (end=begin+1; end<strlen(tmp); end++)
//             {
//                 if (tmp[end] == '[') count++;

//                 if (tmp[end] == ']') count--;

//                 if (count == 0)
//                 {
//                     int j;
//                     for (j=begin+1; j<=end; j++)
//                     {
//                         while (tmp[j] == ' ')
//                         {
//                             int k;
//                             for (k=j; k<strlen(tmp)-1; k++)
//                             {
//                                 tmp[k] = tmp[k+1];
//                             }
//                             end--;
//                             len--;
//                         }
//                     }
//                     break;
//                 }
//             }
//         }
//         else if (tmp[i] == '<')
//         {
//             int count = 1;
//             int begin = i;
//             int end;
//             for (end=begin+1; end<strlen(tmp); end++)
//             {
//                 if (tmp[end] == '<') count++;

//                 if (tmp[end] == '>') count--;

//                 if (count == 0)
//                 {
//                     int j;
//                     for (j=begin+1; j<=end; j++)
//                     {
//                         while (tmp[j] == ' ')
//                         {
//                             int k;
//                             for (k=j; k<strlen(tmp)-1; k++)
//                             {
//                                 tmp[k] = tmp[k+1];
//                             }
//                             end--;
//                             len--;
//                         }
//                     }
//                     break;
//                 }
//             }
//         }
//     }
//     tmp[len] = '\0';
//     return tmp;
// }

// static int _decide_cmd_type(char* text)
// {
//     if (text[0] == '<') // <1-100>
//     {
//         return CMD_INT;
//     }
//     else if (text[0] == 'A') // A.B.C.D/M
//     {
//         return CMD_IPV4;
//     }
//     else if (text[0] == 'X') // X:X:X:X:X:X
//     {
//         return CMD_MACADDR;
//     }
//     else if (_is_all_upper(text))
//     {
//         return CMD_STRING;
//     }
//     return CMD_TOKEN;
// }

// static int _compare_cmd_str(void* data, void* arg)
// {
//     struct cli_cmd* c = (struct cli_cmd*)data;
//     char* str = (char*)arg;
//     bool ret;

//     if (c->type == CMD_TOKEN)
//     {
//         ret = (0 == strcmp(c->str, str));
//     }
//     else if (c->type == CMD_INT)
//     {
//         // long lbound, ubound;
//         // sscanf(c->str, "<%ld-%ld>", &lbound, &ubound);

//         long value = strtol(str, NULL, 10);
//         if ((value == ERANGE) || (value > c->ubound) || (value < c->lbound))
//         {
//             ret = false;
//         }
//         else
//         {
//             ret = true;
//         }
//     }
//     else if (c->type == CMD_IPV4)
//     {
//         int ipv4[4];
//         int num = sscanf(str, "%d.%d.%d.%d", &ipv4[0], &ipv4[1], &ipv4[2], &ipv4[3]);
//         ret = (num == 4) ? true : false;
//     }
//     else if (c->type == CMD_MACADDR)
//     {
//         int mac[6];
//         int num = sscanf(str, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
//         ret = (num == 6) ? true : false;
//     }
//     else if (c->type == CMD_STRING)
//     {
//         ret = (str != NULL) ? true : false ;
//     }
//     return ret;
// }

// static struct array* _get_fit_cmds(struct array* source, char* str)
// {
//     struct array* ret = array_create(NULL);

//     if ((source == NULL) || (str == NULL)) return ret;

//     struct cli_cmd* cmd = (struct cli_cmd*)array_find(source, _compare_cmd_str, str);
//     if (cmd)
//     {
//         // 如果有 100% 比對到的話，那就是只有一個 (包含所有的 cmd type)
//         array_add(ret, cmd);
//     }
//     else
//     {
//         // 沒有 100% 正確的  就會以 token 為根據，找出有可能的 completions
//         int i;
//         for (i=0; i<source->num; i++)
//         {
//             cmd = (struct cli_cmd*)source->datas[i];
//             if ((cmd->type == CMD_TOKEN) && (cmd->str == strstr(cmd->str, str)))
//             {
//                 array_add(ret, cmd);
//             }
//         }
//     }
//     return ret;
// }

// static int _compare_node_id(void* data, void* arg)
// {
//     struct cli_node* n = (struct cli_node*)data;
//     int* id = (int*)arg;
//     return (n->id == *id) ? 1 : 0 ;
// }

// static void _clean_node(void* data)
// {
//     if (data)
//     {
//         struct cli_node* n = (struct cli_node*)data;
//         if (n->prompt) free(n->prompt);
//         array_release(n->cmds);
//         free(data);
//     }
// }

// static struct cli_node* _get_node(int id)
// {
//     return array_find(_nodes, _compare_node_id, &id);
// }

// static void _clean_cmd(void* data)
// {
//     if (data)
//     {
//         struct cli_cmd* c = (struct cli_cmd*)data;
//         if (c->str) free(c->str);
//         if (c->desc) free(c->desc);
//         array_release(c->sub_cmds);
//         free(data);
//     }
// }

// static int _compare_str(void* data, void* arg)
// {
//     struct cli_cmd* cmd = (struct cli_cmd*)data;
//     char* str = (char*)arg;
//     return (0 == strcmp(cmd->str, str)) ? 1 : 0;
// }

// static struct cli_cmd* _install_cmd_element(struct cli_cmd* parent, char* string, cli_func func, int node_id, char* desc)
// {
//     CHECK_IF(string == NULL, return NULL, "string is null");

//     struct cli_node* node = _get_node(node_id);
//     CHECK_IF(node == NULL, return NULL, "node with id = %d is not in the list", node_id);

//     char* pre_process_str = _pre_process_string(string);
//     struct array* cmds = (parent) ? (parent->sub_cmds) : (node->cmds);
//     struct cli_cmd* c = (struct cli_cmd*)array_find(cmds, _compare_str, pre_process_str);
//     if (c != NULL)
//     {
//         free(pre_process_str);
//         c->func = func;
//         return c;
//     }

//     int type    = _decide_cmd_type(pre_process_str);
//     long ubound = 0;
//     long lbound = 0;
//     if (type == CMD_INT)
//     {
//         if (2 != sscanf(pre_process_str, "<%ld-%ld>", &lbound, &ubound))
//         {
//             derror("parse %s failed", pre_process_str);
//             free(pre_process_str);
//             return NULL;
//         }
//     }

//     c         = calloc(sizeof(*c), 1);
//     c->str    = pre_process_str;
//     c->type   = type;
//     c->desc   = (desc) ? strdup(desc) : strdup(" ") ;
//     c->func   = func;
//     c->lbound = lbound;
//     c->ubound = ubound;
//     c->sub_cmds = array_create(_clean_cmd);

//     array_add(cmds, c);
//     return c;
// }

// int cli_install_node(int id, char* prompt)
// {
//     CHECK_IF(prompt == NULL, return CMD_OK, "prompt is null");

//     struct cli_node* node = _get_node(id);
//     CHECK_IF(node != NULL, return CMD_FAIL, "id = %d is already in the list", id);

//     node         = calloc(sizeof(*node), 1);
//     node->id     = id;
//     node->prompt = strdup(prompt);
//     node->cmds   = array_create(_clean_cmd);

//     array_add(_nodes, node);
//     return CMD_OK;
// }

// void cli_install_regular(cli_func func)
// {
//     _regular_fn = func;
// }

// void _show_cmd(struct cli_cmd* cmd, int layer)
// {
//     static char* spaces = "                                                     ";

//     CHECK_IF(cmd == NULL, return, "cmd is null");

//     dprint("%.*s %s (%s)", layer*2, spaces, cmd->str, cmd->desc);

//     struct cli_cmd* sub;
//     int i;
//     for (i=0; i<cmd->sub_cmds->num; i++)
//     {
//         sub = (struct cli_cmd*)(cmd->sub_cmds->datas[i]);
//         _show_cmd(sub, layer+1);
//     }
//     return;
// }

// void cli_show_cmds(int node_id)
// {
//     struct cli_node* node = _get_node(node_id);
//     CHECK_IF(node == NULL, return, "node with id = %d is not in the list", node_id);

//     struct cli_cmd* cmd;
//     int i;
//     for (i=0; i<node->cmds->num; i++)
//     {
//         cmd = (struct cli_cmd*)(node->cmds->datas[i]);
//         _show_cmd(cmd, 1);
//     }
//     return;
// }

// struct array* cli_get_completions(int node_id, char* string, char** output)
// {
//     CHECK_IF(string == NULL, return NULL, "string is null");
//     CHECK_IF(output == NULL, return NULL, "output is null");

//     struct cli_node* node = _get_node(node_id);
//     CHECK_IF(node == NULL, return NULL, "node with id = %d is not in the list", node_id);

//     struct array* cmds      = node->cmds;
//     struct array* str_array = _string_to_array(string, " \r\n\t");
//     struct array* fit_cmds  = NULL;
//     struct array* ret       = array_create(NULL);

//     *output = calloc(CMD_MAX_INPUT_SIZE, 1);
//     *output[0] = '\0';

//     int i;
//     for (i=0; i<str_array->num; i++)
//     {
//         fit_cmds = _get_fit_cmds(cmds, (char*)str_array->datas[i]);
//         CHECK_IF(fit_cmds == NULL, break, "_get_fit_cmds failed");

//         if (fit_cmds->num == 0) // 連一個可能的都沒有
//         {
//             strcat(*output, (char*)str_array->datas[i]);
//             strcat(*output, " ");
//             cmds = NULL;
//         }
//         else if (fit_cmds->num == 1) // 100% 比對成功，只會找到一個
//         {
//             struct cli_cmd* fit_cmd = (struct cli_cmd*)(fit_cmds->datas[0]);
//             if (fit_cmd->type == CMD_TOKEN)
//             {
//                 // 是 token 的話，會自動把字串補齊
//                 strcat(*output, fit_cmd->str);
//             }
//             else
//             {
//                 // 其他的話就照原本的輸入
//                 strcat(*output, (char*)str_array->datas[i]);
//             }
//             strcat(*output, " ");
//             cmds = fit_cmd->sub_cmds;
//         }
//         else // 非 100% 比對成功，找到多個有可能的
//         {
//             if (i == fit_cmds->num-1)
//             {
//                 // 最後一次  把可能的 completions 結果填到 ret 當中
//                 strcat(*output, (char*)fit_cmds->datas[i]);
//                 int j;
//                 struct cli_cmd* cmd;
//                 for (j=0; j<fit_cmds->num; j++)
//                 {
//                     cmd = (struct cli_cmd*)(fit_cmds->datas[i]);
//                     array_add(ret, cmd->str);
//                 }
//             }
//             else
//             {
//                 // 非最後一次  視同找不到
//                 strcat(*output, (char*)str_array->datas[i]);
//                 strcat(*output, " ");
//                 cmds = NULL;
//             }
//         }
//         array_release(fit_cmds);
//         fit_cmds = NULL;
//     }
//     array_release(str_array);
//     return ret;
// }

// int cli_excute_cmd(int node_id, char* string)
// {
//     CHECK_IF(string == NULL, return CMD_FAIL, "string is null");
//     CHECK_IF(strlen(string) > CMD_MAX_INPUT_SIZE, return CMD_FAIL, "string len = %d invalid", strlen(string));

//     struct cli_node* node = _get_node(node_id);
//     CHECK_IF(node == NULL, return CMD_FAIL, "node with id = %d is not in the list", node_id);

//     struct array* str_array = _string_to_array(string, " \r\t\n");
//     CHECK_IF(str_array == NULL, return CMD_FAIL, "_string_to_array failed");

//     struct array* cmds      = node->cmds;
//     struct array* fit_cmds  = NULL;
//     struct array* arguments = array_create(NULL);
//     int i;
//     bool running = true;
//     for (i=0; i<str_array->num && running; i++)
//     {
//         fit_cmds = _get_fit_cmds(cmds, (char*)str_array->datas[i]);
//         if (fit_cmds->num == 1)
//         {
//             // 100% 比對成功，只有一個
//             struct cli_cmd* c = (struct cli_cmd*)fit_cmds->datas[0];
//             if (c->type != CMD_TOKEN)
//             {
//                 // 不是 token 的話，一律視為變數
//                 array_add(arguments, str_array->datas[i]);
//             }

//             if (c->func && (i == str_array->num-1))
//             {
//                 // 如果是最後一個  而且有 callback function，執行，然後結束
//                 c->func(NULL, arguments->num, (char**)arguments->datas);
//                 running = false;
//             }
//             // 繼續比下一層
//             cmds = c->sub_cmds;
//         }
//         else
//         {
//             // 沒有比到 (比到多個算是沒比到啦) 執行 regular callback
//             // 把完整的輸入字串當成變數丟進 regular callback
//             if (_regular_fn) _regular_fn(NULL, 1, &string);

//             running = false;
//         }
//         array_release(fit_cmds);
//     }
//     array_release(str_array);
//     array_release(arguments);
//     return CMD_OK;
// }

// static int _install_wrapper(struct cli_cmd* parent, struct array* str_array, cli_func func, int node_id, struct array* desc_array, int idx)
// {
//     CHECK_IF(str_array == NULL, return CMD_FAIL, "str_array is null");
//     CHECK_IF(idx < 0, return CMD_FAIL, "idx = %d invalid", idx);

//     char* str = (char*)str_array->datas[idx];
//     char* desc = (idx > desc_array->num) ? NULL : (char*)desc_array->datas[idx];

//     if (IS_LAST_CMD(str_array, idx))
//     {
//         if (IS_OPT_CMD(str))
//         {
//             char* content = NULL;
//             sscanf(str, "[%s]", content);

//             _install_cmd_element(parent, content, func, node_id, desc);
//             parent->func = func;
//         }
//         else if (IS_ALT_CMD(str))
//         {
//             struct array* content_array = _string_to_array(str, "(|)");
//             char* content;
//             int i;
//             for (i=0; i<content_array->num; i++)
//             {
//                 content = (char*)content_array->datas[i];
//                 _install_cmd_element(parent, content, func, node_id, desc);
//             }
//             array_release(content_array);

//             if (_is_alt_cmd_with_opt(str))
//             {
//                 parent->func = func;
//             }
//         }
//         else
//         {
//             _install_cmd_element(parent, str, func, node_id, desc);
//         }
//     }
//     else
//     {
//         struct cli_cmd* c = NULL;
//         if (IS_OPT_CMD(str))
//         {
//             char* content = NULL;
//             sscanf(str, "[%s]", content);

//             c = _install_cmd_element(parent, content, NULL, node_id, desc);
//             _install_wrapper(c, str_array, func, node_id, desc_array, idx+1);
//             _install_wrapper(parent, str_array, func, node_id, desc_array, idx+1);
//         }
//         else if (IS_ALT_CMD(str))
//         {
//             struct array* content_array = _string_to_array(str, "(|)");
//             char* content;
//             int i;
//             for (i=0; i<content_array->num; i++)
//             {
//                 content = (char*)content_array->datas[i];
//                 c = _install_cmd_element(parent, content, NULL, node_id, desc);
//                 _install_wrapper(c, str_array, func, node_id, desc_array, idx+1);
//             }
//             array_release(content_array);

//             if (_is_alt_cmd_with_opt(str))
//             {
//                 _install_wrapper(parent, str_array, func, node_id, desc_array, idx+1);
//             }
//         }
//         else
//         {
//             c = _install_cmd_element(parent, str, NULL, node_id, desc);
//             _install_wrapper(c, str_array, func, node_id, desc_array, idx+1);
//         }
//     }
//     return CMD_OK;
// }

// int cli_install_cmd(int node_id, struct cli_cmd_cfg* cfg)
// {
//     CHECK_IF(cfg == NULL, return CMD_FAIL, "cfg is null");
//     CHECK_IF(cfg->func == NULL, return CMD_FAIL, "cfg->func is null");
//     CHECK_IF(cfg->cmd_str == NULL, return CMD_FAIL, "cfg->cmd_str is null");

//     // 先把字串處理過
//     char* dup = _pre_process_string(cfg->cmd_str);

//     // 把字串切成 array
//     struct array* str_array = _string_to_array(dup, " \r\n\t");

//     // 把 descrption 也切成 array
//     struct array* desc_array = array_create(NULL);
//     int i;
//     for (i=0; cfg->descs[i] != NULL; i++)
//     {
//         array_add(desc_array, cfg->descs[i]);
//     }

//     // 開始 install cmd
//     _install_wrapper(NULL, str_array, cfg->func, node_id, desc_array, 0);

//     array_release(desc_array);
//     array_release(str_array);
//     free(dup);
//     return CMD_OK;
// }

// int cli_sys_init(void)
// {
//     _nodes = array_create(_clean_node);
//     return CMD_OK;
// }

// void cli_sys_uninit(void)
// {
//     array_release(_nodes);
//     _nodes = NULL;
// }
