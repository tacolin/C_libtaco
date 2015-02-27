#include "list.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static int  _checkList(tList* list)
{
    if (list == NULL) return LIST_FAIL;

    if (list->num > 0)
    {
        if ((list->head == NULL) || (list->tail == NULL)) return LIST_FAIL;
    }
    else // num == 0
    {
        if ((list->head) || (list->tail)) return LIST_FAIL;        
    }

    return LIST_OK;
}

////////////////////////////////////////////////////////////////////////////////

int  list_init(tList* list, tListContentCleanFn clean_fn)
{
    check_if(list == NULL, return LIST_FAIL, "list is null");

    memset(list, 0, sizeof(tList));

    list->cleanfn = clean_fn;

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

int  list_insert(tList* list, void* content)
{
    check_if(list == NULL, return LIST_FAIL, "list is null");
    check_if(content == NULL, return LIST_FAIL, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_FAIL, "_checkList failed");

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

int  list_append(tList* list, void* content)
{
    check_if(list == NULL, return LIST_FAIL, "list is null");
    check_if(content == NULL, return LIST_FAIL, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_FAIL, "_checkList failed");

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

int  list_remove(tList* list, void* content)
{
    check_if(list == NULL, return LIST_FAIL, "list is null");
    check_if(content == NULL, return LIST_FAIL, "content is null");
    check_if(_checkList(list) != LIST_OK, return LIST_FAIL, "_checkList failed");

    tListObj* obj = list->head;
    while (obj)
    {
        if (obj->content == content)
        {
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
        obj = obj->next;
    }

    return LIST_FAIL;
}

void* list_head(tList* list)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    if (list->head == NULL) return NULL;

    return list->head->content;
}

void* list_tail(tList* list)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    if (list->tail == NULL) return NULL;

    return list->tail->content;    
}

void* list_prev(tList* list, void* content)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(content == NULL, return NULL, "content is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    tListObj* obj = list->head;
    void* ret = NULL;

    while (obj)
    {
        if (obj->content == content)
        {
            tListObj* prev = obj->prev;
            if (prev)
            {
                ret = prev->content;
            }
            goto _END;
        }
        obj = obj->next;
    }

_END:
    return ret;
}

void* list_next(tList* list, void* content)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(content == NULL, return NULL, "content is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    tListObj* obj = list->head;
    void* ret = NULL;

    while (obj)
    {
        if (obj->content == content)
        {
            tListObj* next = obj->next;
            if (next)
            {
                ret = next->content;
            }
            goto _END;
        }
        obj = obj->next;
    }

_END:
    return ret;
}

void* list_find(tList* list, tListContentFindFn find_fn, void* arg)
{
    check_if(list == NULL, return NULL, "list is null");
    check_if(_checkList(list) != LIST_OK, return NULL, "_checkList failed");

    tListObj* obj = list->head;
    void* ret = NULL;
    while (obj)
    {
        if (find_fn)
        {
            if (find_fn(obj->content, arg) == LIST_TRUE)
            {
                ret = obj->content;
                goto _END;
            }
        }
        else
        {
            if (obj->content == arg)
            {
                ret = obj->content;
                goto _END;
            }
        }

        obj = obj->next;
    }

_END:
    return ret;
}

int  list_length(tList* list)
{
    check_if(list == NULL, return -1, "list is null");
    check_if(_checkList(list) != LIST_OK, return -1, "_checkList failed");

    return list->num;    
}

