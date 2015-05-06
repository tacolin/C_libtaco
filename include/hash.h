#ifndef _HASH_H_
#define _HASH_H_

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <search.h>

#include "basic.h"
#include "list.h"

#define HASH_OK (0)
#define HASH_FAIL (-1)

#define HASH_FOREACH(ptable, key, value) \
    for(key = hash_first_key(ptable), value = hash_find(ptable, key); \
        key && value; \
        key = hash_next_key(ptable, key), value = hash_find(ptable, key))

struct hash
{
    struct hsearch_data hdata;
    struct list keys;
    int max_num;
    void (*cleanfn)(void* value);
    int is_init;
};

int hash_init(struct hash* h, int max_num, void (*cleanfn)(void*));
int hash_uninit(struct hash* h);

int hash_add(struct hash *h, char* key, void* value);
int hash_del(struct hash *h, char* key, void** pvalue);
int hash_modify(struct hash* h, char* key, void* value);

void* hash_find(struct hash *h, char* key);
int   hash_contains(struct hash *h, char* key);

char* hash_first_key(struct hash* h);
char* hash_next_key(struct hash* h, char* newkey);

#endif //_HASH_H_
