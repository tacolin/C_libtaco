#ifndef _LIST_H_
#define _LIST_H_

#include "basic.h"

////////////////////////////////////////////////////////////////////////////////

#define LIST_FOREACH(pList, _obj, _content) \
    for (_obj = list_headObj(pList), _content = (_obj) ? _obj->content : NULL; \
         _obj && _content; \
         _obj = list_nextObj(pList, _obj), _content = (_obj) ? _obj->content : NULL)

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    LIST_OK = 0,
    LIST_ERROR = -1

} tListStatus;

typedef enum
{
    LIST_TRUE = 1,
    LIST_FALSE = 0

} tListBool;

typedef struct tListObj
{
    struct tListObj* prev;
    struct tListObj* next;

    void* content;

} tListObj;

typedef void (*tListContentCleanFn)(void* content);
typedef tListBool (*tListContentFindFn)(void* content, void* arg);

typedef struct tList
{
    tListObj* head;
    tListObj* tail;

    int num;

    tListContentCleanFn cleanfn;

} tList;

////////////////////////////////////////////////////////////////////////////////

tListStatus list_init(tList* list, tListContentCleanFn cleanfn);
void list_clean(tList* list);

tListStatus list_insert(tList* list, void* content);
tListStatus list_append(tList* list, void* content);

tListStatus list_insertTo(tList* list, void* target, void* content);
tListStatus list_appendTo(tList* list, void* target, void* content);

tListStatus list_remove(tList* list, void* content);

void* list_find(tList* list, tListContentFindFn find_fn, void* arg);

void* list_head(tList* list);
void* list_tail(tList* list);

void* list_prev(tList* list, void* content);
void* list_next(tList* list, void* content);

int  list_length(tList* list);

////////////////////////////////////////////////////////////////////////////////

tListObj* list_findObj(tList* list, tListContentFindFn find_fn, void* arg);
tListObj* list_headObj(tList* list);
tListObj* list_tailObj(tList* list);

tListObj* list_prevObj(tList* list, tListObj* obj);
tListObj* list_nextObj(tList* list, tListObj* obj);

tListStatus list_insertToObj(tList* list, tListObj* target_obj, void* content);
tListStatus list_appendToObj(tList* list, tListObj* target_obj, void* content);

#endif //_LIST_H_
