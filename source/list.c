#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)

#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static int _check_list(struct list* list)
{
    if (list == NULL) return LIST_FAIL;
    if (list->num > 0)
    {
        if ((list->head == NULL) || (list->tail == NULL)) return LIST_FAIL;
    }
    else
    {
        if ((list->head) || (list->tail)) return LIST_FAIL;
    }
    return LIST_OK;
}

int list_init(struct list* list, void (*cleanfn)(void*))
{
    CHECK_IF(list == NULL, return LIST_FAIL, "list is null");
    memset(list, 0, sizeof(struct list));
    list->cleanfn = cleanfn;
    return LIST_OK;
}

void list_clean(struct list* list)
{
    CHECK_IF(list == NULL, return, "list is null");
    struct list_node* node = list_head_node(list);
    struct list_node* next;
    while (node)
    {
        next = list_next_node(node);
        if (list->cleanfn)
        {
            list->cleanfn(node->data);
        }
        free(node);
        node = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->num  = 0;
    return;
}

int list_append(struct list* list, void* data)
{
    CHECK_IF(list == NULL, return LIST_FAIL, "list is null");
    CHECK_IF(data == NULL, return LIST_FAIL, "data is null");
    CHECK_IF(_check_list(list) != LIST_OK, return LIST_FAIL, "_check_list failed");

    struct list_node* node = calloc(sizeof(struct list_node), 1);
    node->data = data;
    if (list->tail)
    {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    else
    {
        list->head = node;
        list->tail = node;
    }
    list->num++;
    return LIST_OK;
}

int list_insert(struct list* list, void* data)
{
    CHECK_IF(list == NULL, return LIST_FAIL, "list is null");
    CHECK_IF(data == NULL, return LIST_FAIL, "data is null");
    CHECK_IF(_check_list(list) != LIST_OK, return LIST_FAIL, "_check_list failed");

    struct list_node* node = calloc(sizeof(struct list_node), 1);
    node->data = data;
    if (list->head)
    {
        list->head->prev = node;
        node->next = list->head;
        list->head = node;
    }
    else
    {
        list->head = node;
        list->tail = node;
    }
    list->num++;
    return LIST_OK;
}

struct list_node* list_find_node(struct list* list, int (*findfn)(void*, void*), void* arg)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(_check_list(list) != LIST_OK, return NULL, "_check_list failed");

    void* node;
    void* data;
    LIST_FOREACH(list, node, data)
    {
        if (findfn)
        {
            if (findfn(data, arg)) break;
        }
        else
        {
            if (data == arg) break;
        }
    }
    return node;
}

int list_remove_node(struct list* list, struct list_node* node)
{
    CHECK_IF(list == NULL, return LIST_FAIL, "list is null");
    CHECK_IF(node == NULL, return LIST_FAIL, "node is null");
    CHECK_IF(_check_list(list) != LIST_OK, return LIST_FAIL, "_check_list failed");

    if (node->prev && node->next) // middle
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    else if (node->next) // head
    {
        node->next->prev = NULL;
        list->head = node->next;
    }
    else if (node->prev) // tail
    {
        node->prev->next = NULL;
        list->tail = node->prev;
    }
    else // both tail and head, list num = 1
    {
        list->tail = NULL;
        list->head = NULL;
    }
    list->num--;
    return LIST_OK;
}

void* list_find(struct list* list, int (*findfn)(void*, void*), void* arg)
{
    struct list_node* node = list_find_node(list, findfn, arg);
    if (node == NULL) return NULL;
    return node->data;
}

int list_remove(struct list* list, void* data)
{
    struct list_node* node = list_find_node(list, NULL, data);
    if (node == NULL) return LIST_FAIL;

    int ret = list_remove_node(list, node);
    free(node);
    return ret;
}

struct list_node* list_head_node(struct list* list)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(_check_list(list) != LIST_OK, return NULL, "_check_list failed");
    return list->head;
}

struct list_node* list_tail_node(struct list* list)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(_check_list(list) != LIST_OK, return NULL, "_check_list failed");
    return list->tail;
}

void* list_head(struct list* list)
{
    struct list_node* node = list_head_node(list);
    if (node == NULL) return NULL;
    return node->data;
}

void* list_tail(struct list* list)
{
    struct list_node* node = list_tail_node(list);
    if (node == NULL) return NULL;
    return node->data;
}

struct list_node* list_prev_node(struct list_node* node)
{
    CHECK_IF(node == NULL, return NULL, "node is null");
    return node->prev;
}

struct list_node* list_next_node(struct list_node* node)
{
    CHECK_IF(node == NULL, return NULL, "node is null");
    return node->next;
}

void* list_prev(struct list* list, void* data)
{
    struct list_node* node = list_find_node(list, NULL, data);
    if (node == NULL) return NULL;
    struct list_node* prev = list_prev_node(node);
    if (prev == NULL) return NULL;

    return prev->data;
}

void* list_next(struct list* list, void* data)
{
    struct list_node* node = list_find_node(list, NULL, data);
    if (node == NULL) return NULL;
    struct list_node* next = list_next_node(node);
    if (next == NULL) return NULL;

    return next->data;
}

int list_num(struct list* list)
{
    CHECK_IF(list == NULL, return -1, "list is null");
    CHECK_IF(_check_list(list) != LIST_OK, return -1, "_check_list failed");
    return list->num;
}

int list_append_after_node(struct list* list, struct list_node* target_node, void* data)
{
    CHECK_IF(list == NULL, return LIST_FAIL, "list is null");
    CHECK_IF(target_node == NULL, return LIST_FAIL, "target_node is null");
    CHECK_IF(data == NULL, return LIST_FAIL, "data is null");
    CHECK_IF(_check_list(list) != LIST_OK, return LIST_FAIL, "_check_list failed");

    if (target_node->next)
    {
        struct list_node* node  = calloc(sizeof(struct list_node), 1);
        node->data              = data;

        target_node->next->prev = node;
        node->next              = target_node->next;

        target_node->next       = node;
        node->prev              = target_node;

        list->num++;
        return LIST_OK;
    }
    else // target is tail
    {
        return list_append(list, data);
    }
}

int list_insert_before_node(struct list* list, struct list_node* target_node, void* data)
{
    CHECK_IF(list == NULL, return LIST_FAIL, "list is null");
    CHECK_IF(target_node == NULL, return LIST_FAIL, "target_node is null");
    CHECK_IF(data == NULL, return LIST_FAIL, "data is null");
    CHECK_IF(_check_list(list) != LIST_OK, return LIST_FAIL, "_check_list failed");

    if (target_node->prev)
    {
        struct list_node* node  = calloc(sizeof(struct list_node), 1);
        node->data              = data;

        target_node->prev->next = node;
        node->prev              = target_node->prev;

        target_node->prev       = node;
        node->next              = target_node;

        list->num++;
        return LIST_OK;
    }
    else // target is head
    {
        return list_insert(list, data);
    }
}

int list_append_after(struct list* list, void* target, void* data)
{
    struct list_node* node = list_find_node(list, NULL, target);
    if (node == NULL) return LIST_FAIL;
    return list_append_after_node(list, node, data);
}

int list_insert_before(struct list* list, void* target, void* data)
{
    struct list_node* node = list_find_node(list, NULL, target);
    if (node == NULL) return LIST_FAIL;
    return list_insert_before_node(list, node, data);
}
