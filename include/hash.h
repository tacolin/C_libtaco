#ifndef _HASH_H_
#define _HASH_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <search.h>

#include "basic.h"
#include "list.h"

////////////////////////////////////////////////////////////////////////////////

#define HASH_OK (0)
#define HASH_FAIL (-1)

#define HASH_FOREACH(ptable, key, value) \
    for(key = hash_first_key(ptable), value = hash_find(ptable, key); \
        key && value; \
        key = hash_next_key(ptable, key), value = hash_find(ptable, key))

////////////////////////////////////////////////////////////////////////////////

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

int hash_init(tHashTable* tbl, int max_num, tHashValueCleanFn cleanfn);
int hash_uninit(tHashTable* tbl);

int hash_add(tHashTable *tbl, char* key, void* value);
int hash_del(tHashTable *tbl, char* key, void** pvalue);
int hash_modify(tHashTable* tbl, char* key, void* value);

void* hash_find(tHashTable *tbl, char* key);
int   hash_contains(tHashTable *tbl, char* key);

char* hash_first_key(tHashTable* tbl);
char* hash_next_key(tHashTable* tbl, char* newkey);

#endif //_HASH_H_
