#ifndef _LIST_H_
#define _LIST_H_

#include "basic.h"

////////////////////////////////////////////////////////////////////////////////

#define LIST_OK   0
#define LIST_FAIL -1

////////////////////////////////////////////////////////////////////////////////

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

int  list_init(tList* list, tListContentCleanFn clean_fn);
void list_clean(tList* list);

int  list_insert(tList* list, void* content);
int  list_append(tList* list, void* content);

int  list_remove(tList* list, void* content);

void* list_find(tList* list, tListContentFindFn find_fn, void* arg);

void* list_head(tList* list);
void* list_tail(tList* list);

void* list_prev(tList* list, void* content);
void* list_next(tList* list, void* content);

int  list_length(tList* list);

#endif //_LIST_H_