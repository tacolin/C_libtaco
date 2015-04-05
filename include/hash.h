#ifndef _HASH_H_
#define _HASH_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <search.h>

#include "basic.h"
#include "list.h"

////////////////////////////////////////////////////////////////////////////////

#define HASH_FOREACH(ptable, key, value) \
    for(key = hash_first_key(ptable), value = hash_find(ptable, key); \
        key && value; \
        key = hash_next_key(ptable, key), value = hash_find(ptable, key))

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    HASH_OK = 0,
    HASH_ERROR = -1

} tHashStatus;

typedef void (*tHashValueCleanFn)(void* value);

typedef struct tHashTable
{
    struct hsearch_data hdata;

    tHashValueCleanFn cleanfn;

    tList keys;

    int max_num;
    int is_init;

} tHashTable;

////////////////////////////////////////////////////////////////////////////////

tHashStatus hash_init(tHashTable* tbl, int max_num, tHashValueCleanFn cleanfn);
tHashStatus hash_uninit(tHashTable* tbl);

tHashStatus hash_add(tHashTable *tbl, char* key, void* value);
tHashStatus hash_del(tHashTable *tbl, char* key, void** pvalue);
tHashStatus hash_modify(tHashTable* tbl, char* key, void* value);

void* hash_find(tHashTable *tbl, char* key);
int   hash_contains(tHashTable *tbl, char* key);

char* hash_first_key(tHashTable* tbl);
char* hash_next_key(tHashTable* tbl, char* newkey);

#endif //_HASH_H_