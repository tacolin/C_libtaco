#include <stdio.h>
#include <string.h>

#include "fast_list.h"

#define FLIST_GUARD_CODE (0x55665566)

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static int _check_flist(struct flist* list)
{
    if (list == NULL)    return FLIST_FAIL;
    if (list->is_init != 1) return FLIST_FAIL;

    if ((list->head) && (list->head->prev)) return FLIST_FAIL;
    if ((list->tail) && (list->tail->next)) return FLIST_FAIL;

    return FLIST_OK;
}

static int _is_node_unused(void* node)
{
    struct flist_hdr* hdr = (struct flist_hdr*)node;
    if (hdr == NULL) return 0;
    if ((hdr->guard_code == FLIST_GUARD_CODE) && (hdr->enable == 1)) return 0;

    return 1;
}

static int _is_node_used(void* node)
{
    if (node == NULL) return 0;
    return _is_node_unused(node) ? 0 : 1 ;
}

int flist_init(struct flist* list, void (*cleanfn)(void*))
{
    CHECK_IF(list == NULL, return FLIST_FAIL, "list is null");

    memset(list, 0, sizeof(struct flist));
    list->cleanfn = cleanfn;
    list->is_init = 1;
    return FLIST_OK;
}

void flist_clean(struct flist* list)
{
    CHECK_IF(_check_flist(list) != FLIST_OK, return, "_check_flist failed");

    struct flist_hdr* node = flist_head(list);
    struct flist_hdr* next;
    while (node)
    {
        next = flist_next(list, node);
        if (list->cleanfn)
        {
            list->cleanfn(node);
        }
        node->prev       = NULL;
        node->next       = NULL;
        node->enable     = 0;
        node->guard_code = 0;

        node = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->num  = 0;
    return;
}

int flist_append(struct flist* list, void* node)
{
    CHECK_IF(list == NULL, return FLIST_FAIL, "list is null");
    CHECK_IF(node == NULL, return FLIST_FAIL, "node is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return FLIST_FAIL, "_check_flist failed");
    CHECK_IF(_is_node_used(node), return FLIST_FAIL, "node is already added to some list");

    struct flist_hdr* hdr = (struct flist_hdr*)node;
    hdr->enable     = 1;
    hdr->guard_code = FLIST_GUARD_CODE;
    hdr->next       = NULL;
    if (list->tail)
    {
        list->tail->next = hdr;
        hdr->prev        = list->tail;
        list->tail       = hdr;
    }
    else // list is empty
    {
        list->head = hdr;
        list->tail = hdr;
        hdr->prev  = NULL;
    }
    list->num++;
    return FLIST_OK;
}

int flist_insert(struct flist* list, void* node)
{
    CHECK_IF(list == NULL, return FLIST_FAIL, "list is null");
    CHECK_IF(node == NULL, return FLIST_FAIL, "node is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return FLIST_FAIL, "_check_flist failed");
    CHECK_IF(_is_node_used(node), return FLIST_FAIL, "node is already added to some list");

    struct flist_hdr* hdr = (struct flist_hdr*)node;
    hdr->enable     = 1;
    hdr->guard_code = FLIST_GUARD_CODE;
    hdr->prev       = NULL;
    if (list->head)
    {
        list->head->prev = hdr;
        hdr->next        = list->head;
        list->head       = hdr;
    }
    else // list is empty
    {
        list->head = hdr;
        list->tail = hdr;
        hdr->next  = NULL;
    }
    list->num++;
    return FLIST_OK;
}

void* flist_find(struct flist* list, int (*findfn)(void* node, void* arg), void* arg)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(findfn == NULL, return NULL, "findfn is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return NULL, "_check_flist failed");

    void* node = NULL;
    FLIST_FOREACH(list, node)
    {
        if (findfn(node, arg)) break;
    }
    return node;
}

int flist_remove(struct flist* list, void* node)
{
    CHECK_IF(list == NULL, return FLIST_FAIL, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return FLIST_FAIL, "_check_flist failed");
    CHECK_IF(node == NULL, return FLIST_FAIL, "node is null");
    CHECK_IF(_is_node_unused(node), return FLIST_FAIL, "node is not added to any list");

    struct flist_hdr* hdr = (struct flist_hdr*)node;
    if (hdr->prev)
    {
        if (hdr->next)
        {
            hdr->prev->next = hdr->next;
            hdr->next->prev = hdr->prev;
        }
        else // node is tail
        {
            CHECK_IF(list->tail != hdr, return FLIST_FAIL, "node is tail but not in this list");

            list->tail       = hdr->prev;
            list->tail->next = NULL;
        }
    }
    else
    {
        if (hdr->next) // node is head
        {
            CHECK_IF(list->head != hdr, return FLIST_FAIL, "node is head but not in this list");

            list->head       = hdr->next;
            list->head->prev = NULL;
        }
        else // node is both tail and head, list has only one node
        {
            CHECK_IF(list->tail != hdr, return FLIST_FAIL, "node is tail but not in this list");
            CHECK_IF(list->head != hdr, return FLIST_FAIL, "node is head but not in this list");

            list->head = NULL;
            list->tail = NULL;
        }
    }
    hdr->next       = NULL;
    hdr->prev       = NULL;
    hdr->enable     = 0;
    hdr->guard_code = 0;
    list->num--;
    return FLIST_OK;
}

void* flist_head(struct flist* list)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return NULL, "_check_flist failed");
    return list->head;
}

void* flist_tail(struct flist* list)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return NULL, "_check_flist failed");
    return list->tail;
}

void* flist_prev(struct flist* list, void* node)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return NULL, "_check_flist failed");
    CHECK_IF(node == NULL, return NULL, "node is null");
    CHECK_IF(_is_node_unused(node), return NULL, "node is not added to any list");

    struct flist_hdr* hdr = (struct flist_hdr*)node;
    return hdr->prev;
}

void* flist_next(struct flist* list, void* node)
{
    CHECK_IF(list == NULL, return NULL, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return NULL, "_check_flist failed");
    CHECK_IF(node == NULL, return NULL, "node is null");
    CHECK_IF(_is_node_unused(node), return NULL, "node is not added to any list");

    struct flist_hdr* hdr = (struct flist_hdr*)node;
    return hdr->next;
}

int flist_append_after(struct flist* list, void* target, void* node)
{
    CHECK_IF(list == NULL, return FLIST_FAIL, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return FLIST_FAIL, "_check_flist failed");
    CHECK_IF(target == NULL, return FLIST_FAIL, "target is null");
    CHECK_IF(_is_node_unused(target), return FLIST_FAIL, "target is not added to any list");
    CHECK_IF(node == NULL, return FLIST_FAIL, "node is null");
    CHECK_IF(_is_node_used(node), return FLIST_FAIL, "node is already added to some list");

    struct flist_hdr* target_hdr = (struct flist_hdr*)target;
    struct flist_hdr* node_hdr = (struct flist_hdr*)node;
    if (target_hdr->next)
    {
        node_hdr->next         = target_hdr->next;
        node_hdr->prev         = target_hdr;
        target_hdr->next->prev = node_hdr;
        target_hdr->next       = node_hdr;
    }
    else // target is tail
    {
        node_hdr->next   = NULL;
        node_hdr->prev   = target_hdr;
        target_hdr->next = node_hdr;
        list->tail       = node_hdr;
    }
    node_hdr->enable     = 1;
    node_hdr->guard_code = FLIST_GUARD_CODE;
    list->num++;
    return FLIST_OK;
}

int flist_insert_before(struct flist* list, void* target, void* node)
{
    CHECK_IF(list == NULL, return FLIST_FAIL, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return FLIST_FAIL, "_check_flist failed");
    CHECK_IF(target == NULL, return FLIST_FAIL, "target is null");
    CHECK_IF(_is_node_unused(target), return FLIST_FAIL, "target is not added to any list");
    CHECK_IF(node == NULL, return FLIST_FAIL, "node is null");
    CHECK_IF(_is_node_used(node), return FLIST_FAIL, "node is already added to some list");

    struct flist_hdr* target_hdr = (struct flist_hdr*)target;
    struct flist_hdr* node_hdr = (struct flist_hdr*)node;
    if (target_hdr->prev)
    {
        node_hdr->next         = target_hdr;
        node_hdr->prev         = target_hdr->prev;
        target_hdr->prev->next = node_hdr;
        target_hdr->prev       = node_hdr;
    }
    else // target is head
    {
        node_hdr->prev   = NULL;
        node_hdr->next   = target_hdr;
        target_hdr->prev = node_hdr;
        list->head       = node_hdr;
    }
    node_hdr->enable     = 1;
    node_hdr->guard_code = FLIST_GUARD_CODE;
    list->num++;
    return FLIST_OK;
}

int flist_num(struct flist* list)
{
    CHECK_IF(list == NULL, return -1, "list is null");
    CHECK_IF(_check_flist(list) != FLIST_OK, return -1, "_check_flist failed");
    return list->num;
}
