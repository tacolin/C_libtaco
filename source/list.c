#include "list.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static tListStatus _checkList(tList* list)
{
    if (list == NULL) return LIST_ERROR;

    if (list->num > 0)
    {
        if ((list->head == NULL) || (list->tail == NULL)) return LIST_ERROR;
    }
    else // num == 0
    {
        if ((list->head) || (list->tail)) return LIST_ERROR;
    }

    return LIST_OK;
}

////////////////////////////////////////////////////////////////////////////////

tListStatus list_init(tList* list, tListContentCleanFn cleanfn)
{
    check_if(list == NULL, return LIST_ERROR, "list is null");

    memset(list, 0, sizeof(tList));

    list->cleanfn = cleanfn;

    return LIST_OK;
}

void list_clean(tList* list)
{
    check_if(list == NULL, return, "list is null");

    tListObj* obj = list->head;
    tListObj* next;

    while (obj)
    {
        next = obj->next;
        if (list->cleanfn)
        {
            list->cleanfn(obj->content);
        }
        free(obj);
        obj = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->num  = 0;

    return;
}

tListStatus list_insert(tList* list, void* content)
{
    check_if(list == NULL, return LIST_ERROR, "list is null");
    check_if(content == NULL, return LIST_ERROR, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_ERROR, "_checkList failed");
    check_if(list_find(list, NULL, content) != NULL, return LIST_ERROR, "content is already in list");

    tListObj* obj = calloc(sizeof(tListObj), 1);
    obj->content = content;

    if (list->head)
    {
        list->head->prev = obj;
        obj->next = list->head;
        list->head = obj;
    }
    else
    {
        list->head = obj;
        list->tail = obj;
    }

    list->num++;

    return LIST_OK;
}

tListStatus list_append(tList* list, void* content)
{
    check_if(list == NULL, return LIST_ERROR, "list is null");
    check_if(content == NULL, return LIST_ERROR, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_ERROR, "_checkList failed");
    // check_if(list_find(list, NULL, content) != NULL, return LIST_ERROR, "content is already in list");

    tListObj* obj = calloc(sizeof(tListObj), 1);
    obj->content = content;

    if (list->tail)
    {
        list->tail->next = obj;
        obj->prev = list->tail;
        list->tail = obj;
    }
    else
    {
        list->head = obj;
        list->tail = obj;
    }

    list->num++;

    return LIST_OK;
}

tListStatus list_remove(tList* list, void* content)
{
    check_if(list == NULL, return LIST_ERROR, "list is null");
    check_if(content == NULL, return LIST_ERROR, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_ERROR, "_checkList failed");

    tListObj* obj = list_findObj(list, NULL, content);
    check_if(obj == NULL, return LIST_ERROR, "content is not in this list");

    if (obj->prev && obj->next) // middle
    {
        obj->prev->next = obj->next;
        obj->next->prev = obj->prev;
    }
    else if (obj->next) // head
    {
        obj->next->prev = NULL;
        list->head = obj->next;
    }
    else if (obj->prev) // tail
    {
        obj->prev->next = NULL;
        list->tail = obj->prev;
    }
    else // both tail and head, list num = 1
    {
        list->tail = NULL;
        list->head = NULL;
    }

    list->num--;

    free(obj);

    return LIST_OK;
}

tListObj* list_headObj(tList* list)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    return list->head;
}

void* list_head(tList* list)
{
    tListObj* obj = list_headObj(list);
    return (obj != NULL) ? obj->content : NULL;
}

tListObj* list_tailObj(tList* list)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    return list->tail;
}

void* list_tail(tList* list)
{
    tListObj* obj = list_tailObj(list);
    return (obj != NULL) ? obj->content : NULL;
}

tListObj* list_prevObj(tList* list, tListObj* obj)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(obj == NULL, return NULL, "obj is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    return obj->prev;
}

void* list_prev(tList* list, void* content)
{
    tListObj* obj = list_findObj(list, NULL, content);
    check_if(obj == NULL, return NULL, "content is not in this list");

    tListObj* prev_obj = list_prevObj(list, obj);
    return (prev_obj != NULL) ? prev_obj->content : NULL;
}

tListObj* list_nextObj(tList* list, tListObj* obj)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(obj == NULL, return NULL, "obj is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    return obj->next;
}

void* list_next(tList* list, void* content)
{
    tListObj* obj = list_findObj(list, NULL, content);
    check_if(obj == NULL, return NULL, "content is not in this list");

    tListObj* next_obj = list_nextObj(list, obj);
    return (next_obj != NULL) ? next_obj->content : NULL;
}

tListObj* list_findObj(tList* list, tListContentFindFn find_fn, void* arg)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    tListObj* obj = list->head;
    while (obj)
    {
        if (find_fn)
        {
            if (find_fn(obj->content, arg) == LIST_TRUE)
            {
                goto _END;
            }
        }
        else
        {
            if (obj->content == arg)
            {
                goto _END;
            }
        }

        obj = obj->next;
    }

_END:
    return obj;
}

void* list_find(tList* list, tListContentFindFn find_fn, void* arg)
{
    tListObj* obj = list_findObj(list, find_fn, arg);
    return (obj != NULL) ? obj->content : NULL;
}

int  list_length(tList* list)
{
    check_if(list == NULL, return -1, "list is null");
    check_if(_checkList(list) != LIST_OK, return -1, "_checkList failed");

    return list->num;
}

tListStatus list_insertToObj(tList* list, tListObj* target_obj, void* content)
{
    check_if(list == NULL, return LIST_ERROR, "list is null");
    check_if(content == NULL, return LIST_ERROR, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_ERROR, "_checkList failed");
    check_if(target_obj == NULL, return LIST_ERROR, "target_obj is null");
    check_if(list_find(list, NULL, content) != NULL, return LIST_ERROR, "content is already in list");
    check_if(list_findObj(list, NULL, target_obj) == NULL, return LIST_ERROR, "target_obj is not in list");

    if (target_obj->prev)
    {
        tListObj* newobj = calloc(sizeof(tListObj), 1);
        newobj->content = content;

        target_obj->prev->next = newobj;
        newobj->prev = target_obj->prev;

        target_obj->prev = newobj;
        newobj->next = target_obj;
        list->num++;

        return LIST_OK;
    }
    else // target is head
    {
        return list_insert(list, content);
    }
}

tListStatus list_insertTo(tList* list, void* target, void* content)
{
    tListObj* obj = list_findObj(list, NULL, target);
    check_if(obj == NULL, return LIST_ERROR, "target is not in this list");

    return list_insertToObj(list, obj, content);
}

tListStatus list_appendToObj(tList* list, tListObj* target_obj, void* content)
{
    check_if(list == NULL, return LIST_ERROR, "list is null");
    check_if(target_obj == NULL, return LIST_ERROR, "target_obj is null");
    check_if(content == NULL, return LIST_ERROR, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_ERROR, "_checkList failed");
    check_if(list_find(list, NULL, content) != NULL, return LIST_ERROR, "content is already in list");
    check_if(list_findObj(list, NULL, target_obj) == NULL, return LIST_ERROR, "target_obj is not in list");

    if (target_obj->next)
    {
        tListObj* newobj = calloc(sizeof(tListObj), 1);
        newobj->content = content;

        target_obj->next->prev = newobj;
        newobj->next = target_obj->next;

        target_obj->next = newobj;
        newobj->prev = target_obj;
        list->num++;

        return LIST_OK;
    }
    else // target is tail
    {
        return list_append(list, content);
    }
}

tListStatus list_appendTo(tList* list, void* target, void* content)
{
    tListObj* obj = list_findObj(list, NULL, target);
    check_if(obj == NULL, return LIST_ERROR, "target is not in this list");

    return list_appendToObj(list, obj, content);
}
