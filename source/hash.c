#include "hash.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static tListBool _compareKey(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    const char* key1 = (const char*)content;
    const char* key2 = (const char*)arg;

    return (0 == strcmp(key1, key2)) ? LIST_TRUE : LIST_FALSE;
}

////////////////////////////////////////////////////////////////////////////////

tHashStatus hash_init(tHashTable* tbl, int max_num, tHashValueCleanFn cleanfn)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(max_num <= 0, return HASH_ERROR, "max_num = %d invalid", max_num);

    memset(tbl, 0, sizeof(tHashTable));

    tListStatus list_ret = list_init(&tbl->keys, free);
    check_if(list_ret != LIST_OK, return HASH_ERROR, "list init failed");

    int ret = hcreate_r(max_num, &tbl->data);
    check_if(ret == 0, return HASH_ERROR, "hcreate_r failed");

    tbl->max_num = max_num;
    tbl->cleanfn = cleanfn;
    tbl->is_init = 1;

    return HASH_OK;
}

tHashStatus hash_uninit(tHashTable* tbl)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(tbl->is_init != 1, return HASH_ERROR, "tbl is not init yet");

    if (tbl->cleanfn)
    {
        char* key = NULL;
        void* value = 0;
        HASH_FOREACH(tbl, key, value)
        {
            if (value) tbl->cleanfn(value);
        }
    }

    hdestroy_r(&tbl->data);
    list_clean(&tbl->keys);

    tbl->is_init = 0;
    return HASH_OK;
}

tHashStatus hash_add(tHashTable *tbl, char* key, void* value)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(key == NULL, return HASH_ERROR, "key is null");
    check_if(value == NULL, return HASH_ERROR, "value is null");
    check_if(tbl->is_init != 1, return HASH_ERROR, "tbl is not init yet");

    int ret = hash_contains(tbl, key);
    check_if(ret != 0, return HASH_ERROR, "key = %s is already added to table", key);

    ENTRY e = {.key = key, .data = value};
    ENTRY *ep = NULL;

    ret = hsearch_r(e, ENTER, &ep, &tbl->data);
    check_if(ret == 0, return HASH_ERROR, "hsearch_r ENTER failed, key = %s", key);

    char* newkey = strdup(key);
    check_if(newkey == NULL, goto _ERROR, "strdup failed");

    tListStatus list_ret = list_append(&tbl->keys, newkey);
    check_if(list_ret != LIST_OK, goto _ERROR, "list_append failed");

    return HASH_OK;

_ERROR:
    if (newkey) free(newkey);

    e.key = key;
    e.data = NULL;
    ep = &e;

    hsearch_r(e, ENTER, &ep, &tbl->data);

    return HASH_ERROR;
}

tHashStatus hash_del(tHashTable *tbl, char* key, void** pvalue)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(key == NULL, return HASH_ERROR, "key is null");
    check_if(pvalue == NULL, return HASH_ERROR, "pvalue is null");
    check_if(tbl->is_init != 1, return HASH_ERROR, "tbl is not init yet");

    ENTRY e = {.key = key};
    ENTRY *ep = NULL;

    hsearch_r(e, FIND, &ep, &tbl->data);
    if (ep && ep->data)
    {
        *pvalue = ep->data;
        ep->data = NULL;
    }
    else 
    {
        derror("key %s is not in the table", key);
        return HASH_ERROR;        
    }

    char* newkey = list_find(&tbl->keys, _compareKey, key);
    check_if(newkey == NULL, return HASH_ERROR, "list_find failed");
    list_remove(&tbl->keys, newkey);
    free(newkey);

    return HASH_OK;    
}

void* hash_find(tHashTable *tbl, char* key)
{
    check_if(tbl == NULL, return NULL, "tbl is null");
    // check_if(key == NULL, return NULL, "key is null");
    check_if(tbl->is_init != 1, return NULL, "tbl is not init yet");

    if (key == NULL) return NULL;

    ENTRY e = {.key = key};
    ENTRY *ep = NULL;

    hsearch_r(e, FIND, &ep, &tbl->data);

    return (ep == NULL) ? NULL : ep->data;
}

int  hash_contains(tHashTable *tbl, char* key)
{
    return (NULL != hash_find(tbl, key)) ? 1 : 0;
}

char* hash_first_key(tHashTable* tbl)
{
    return list_head(&tbl->keys);
}

char* hash_next_key(tHashTable* tbl, char* key)
{
    return list_next(&tbl->keys, key);
}
    