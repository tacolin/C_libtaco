#include "hash.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static tListBool _comparekey(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    return (strcmp((char*)content, (char*)arg) == 0) ? LIST_TRUE : LIST_FALSE;
}

static tHashStatus _hcreate(struct hsearch_data* hdata, int max_num)
{
    check_if(hdata == NULL, return HASH_ERROR, "hdata is null");
    check_if(max_num <= 0, return HASH_ERROR, "max_num = %d invalid", max_num);

    int ret = hcreate_r(max_num, hdata);
    check_if(ret == 0, return HASH_ERROR, "hcreate_r failed");

    return HASH_OK;
}

static void _hdestroy(struct hsearch_data* hdata)
{
    check_if(hdata == NULL, return, "hdata is null");
    hdestroy_r(hdata);
}

static tHashStatus _hadd(struct hsearch_data* hdata, char* key, void* value)
{
    check_if(hdata == NULL, return HASH_ERROR, "hdata is null");
    check_if(key == NULL, return HASH_ERROR, "key is null");
    check_if(value == NULL, return HASH_ERROR, "value is null");

    ENTRY item = {
        .key = key,
        .data = value
    };

    ENTRY *pitem = NULL;

    int ret = hsearch_r(item, ENTER, &pitem, hdata);
    check_if(ret == 0, return HASH_ERROR, "hsearch_r ENTER failed");

    pitem->data = value;

    return HASH_OK;
}

static tHashStatus _hdelete(struct hsearch_data* hdata, char* key)
{
    check_if(hdata == NULL, return HASH_ERROR, "hdata is null");
    check_if(key == NULL, return HASH_ERROR, "key is null");

    ENTRY item = {
        .key = key
    };

    ENTRY *pitem = NULL;

    int ret = hsearch_r(item, FIND, &pitem, hdata);
    check_if(ret == 0, return HASH_ERROR, "hsearch_r FIND failed");

    pitem->data = NULL;

    return HASH_OK;
}

static void* _hfind(struct hsearch_data* hdata, char* key)
{
    check_if(hdata == NULL, return NULL, "hdata is null");
    check_if(key == NULL, return NULL, "key is null");

    ENTRY item = {
        .key = key
    };

    ENTRY *pitem = NULL;

    int ret = hsearch_r(item, FIND, &pitem, hdata);
    if (ret == 0) return NULL;
    if (pitem == NULL) return NULL;

    return pitem->data;
}

////////////////////////////////////////////////////////////////////////////////

tHashStatus hash_init(tHashTable* tbl, int max_num, tHashValueCleanFn cleanfn)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(max_num <= 0, return HASH_ERROR, "max_num = %d invalid", max_num);

    memset(tbl, 0, sizeof(tHashTable));

    tListStatus lret = list_init(&(tbl->keys), free);
    check_if(lret != LIST_OK, return HASH_ERROR, "list_init failed");

    tbl->max_num = max_num;
    tbl->cleanfn = cleanfn;

    tHashStatus hret = _hcreate(&(tbl->hdata), max_num);
    check_if(hret != HASH_OK, return HASH_ERROR, "_hcreate failed");

    tbl->is_init = 1;
    return HASH_OK;
}

tHashStatus hash_uninit(tHashTable* tbl)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(tbl->is_init != 1, return HASH_ERROR, "tbl is not init yet");

    if (tbl->cleanfn)
    {
        char* key;
        void* value;
        HASH_FOREACH(tbl, key, value)
        {
            tbl->cleanfn(value);
        }
    }

    list_clean(&(tbl->keys));

    _hdestroy(&(tbl->hdata));

    return HASH_OK;
}

tHashStatus hash_add(tHashTable *tbl, char* key, void* value)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(key == NULL, return HASH_ERROR, "key is null");
    check_if(value == NULL, return HASH_ERROR, "value is null");
    check_if(tbl->is_init != 1, return HASH_ERROR, "tbl is not init yet");

    int num = list_length(&(tbl->keys));
    check_if(num >= tbl->max_num, return HASH_ERROR, "tbl num = %d is full", num);

    int contain = hash_contains(tbl, key);
    check_if(contain, return HASH_ERROR, "key = %s is already added", key);

    tHashStatus hret = _hadd(&(tbl->hdata), key, value);
    check_if(hret != HASH_OK, return HASH_ERROR, "_hadd failed");

    char* newkey = strdup(key);
    check_if(newkey == NULL, goto _ERROR, "strdup failed");

    tListStatus lret = list_append(&(tbl->keys), newkey);
    check_if(lret != LIST_OK, goto _ERROR, "list_append failed");

    return HASH_OK;

_ERROR:
    if (newkey) free(newkey);

    _hdelete(&(tbl->hdata), key);

    return HASH_ERROR;
}

tHashStatus hash_modify(tHashTable* tbl, char* key, void* value)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(key == NULL, return HASH_ERROR, "key is null");
    check_if(value == NULL, return HASH_ERROR, "value is null");
    check_if(tbl->is_init != 1, return HASH_ERROR, "tbl is not init yet");

    int contain = hash_contains(tbl, key);
    check_if(contain == 0, return HASH_ERROR, "key = %s not exist in table", key);

    return _hadd(&(tbl->hdata), key, value);
}

tHashStatus hash_del(tHashTable* tbl, char* key, void** pvalue)
{
    check_if(tbl == NULL, return HASH_ERROR, "tbl is null");
    check_if(key == NULL, return HASH_ERROR, "key is null");
    check_if(pvalue == NULL, return HASH_ERROR, "pvalue is null");
    check_if(tbl->is_init != 1, return HASH_ERROR, "tbl is not init yet");

    char* newkey = list_find(&(tbl->keys), _comparekey, key);
    check_if(newkey == NULL, return HASH_ERROR, "key = %s not exist in keys list", key);

    list_remove(&(tbl->keys), newkey);
    free(newkey);

    *pvalue = _hfind(&(tbl->hdata), key);
    check_if(*pvalue == NULL, return HASH_ERROR, "key = %s not exist in table", key);

    return _hdelete(&(tbl->hdata), key);
}

void* hash_find(tHashTable *tbl, char* key)
{
    check_if(tbl == NULL, return NULL, "tbl is null");
    // check_if(key == NULL, return NULL, "key is null");
    check_if(tbl->is_init != 1, return NULL, "tbl is not init yet");

    if (key == NULL) return NULL;

    return _hfind(&(tbl->hdata), key);
}

int   hash_contains(tHashTable *tbl, char* key)
{
    check_if(tbl == NULL, return 0, "tbl is null");
    check_if(key == NULL, return 0, "key is null");
    check_if(tbl->is_init != 1, return 0, "tbl is not init yet");

    return (NULL != _hfind(&(tbl->hdata), key)) ? 1 : 0;
}

char* hash_first_key(tHashTable* tbl)
{
    check_if(tbl == NULL, return 0, "tbl is null");
    check_if(tbl->is_init != 1, return 0, "tbl is not init yet");

    return list_head(&(tbl->keys));
}

char* hash_next_key(tHashTable* tbl, char* newkey)
{
    check_if(tbl == NULL, return 0, "tbl is null");
    check_if(newkey == NULL, return 0, "newkey is null");
    check_if(tbl->is_init != 1, return 0, "tbl is not init yet");

    return list_next(&(tbl->keys), newkey);
}
