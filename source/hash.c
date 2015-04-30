#include "hash.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static int _comparekey(void* content, void* arg)
{
    check_if(content == NULL, return LIST_FALSE, "content is null");
    check_if(arg == NULL, return LIST_FALSE, "arg is null");

    return (strcmp((char*)content, (char*)arg) == 0) ? LIST_TRUE : LIST_FALSE;
}

static int _hcreate(struct hsearch_data* hdata, int max_num)
{
    check_if(hdata == NULL, return HASH_FAIL, "hdata is null");
    check_if(max_num <= 0, return HASH_FAIL, "max_num = %d invalid", max_num);

    int ret = hcreate_r(max_num, hdata);
    check_if(ret == 0, return HASH_FAIL, "hcreate_r failed");

    return HASH_OK;
}

static void _hdestroy(struct hsearch_data* hdata)
{
    check_if(hdata == NULL, return, "hdata is null");
    hdestroy_r(hdata);
}

static int _hadd(struct hsearch_data* hdata, char* key, void* value)
{
    check_if(hdata == NULL, return HASH_FAIL, "hdata is null");
    check_if(key == NULL, return HASH_FAIL, "key is null");
    check_if(value == NULL, return HASH_FAIL, "value is null");

    ENTRY item = {
        .key = key,
        .data = value
    };

    ENTRY *pitem = NULL;

    int ret = hsearch_r(item, ENTER, &pitem, hdata);
    check_if(ret == 0, return HASH_FAIL, "hsearch_r ENTER failed");

    pitem->data = value;

    return HASH_OK;
}

static int _hdelete(struct hsearch_data* hdata, char* key)
{
    check_if(hdata == NULL, return HASH_FAIL, "hdata is null");
    check_if(key == NULL, return HASH_FAIL, "key is null");

    ENTRY item = {
        .key = key
    };

    ENTRY *pitem = NULL;

    int ret = hsearch_r(item, FIND, &pitem, hdata);
    check_if(ret == 0, return HASH_FAIL, "hsearch_r FIND failed");

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

int hash_init(tHashTable* tbl, int max_num, tHashValueCleanFn cleanfn)
{
    check_if(tbl == NULL, return HASH_FAIL, "tbl is null");
    check_if(max_num <= 0, return HASH_FAIL, "max_num = %d invalid", max_num);

    memset(tbl, 0, sizeof(tHashTable));

    int lret = list_init(&(tbl->keys), free);
    check_if(lret != LIST_OK, return HASH_FAIL, "list_init failed");

    tbl->max_num = max_num;
    tbl->cleanfn = cleanfn;

    int hret = _hcreate(&(tbl->hdata), max_num);
    check_if(hret != HASH_OK, return HASH_FAIL, "_hcreate failed");

    tbl->is_init = 1;
    return HASH_OK;
}

int hash_uninit(tHashTable* tbl)
{
    check_if(tbl == NULL, return HASH_FAIL, "tbl is null");
    check_if(tbl->is_init != 1, return HASH_FAIL, "tbl is not init yet");

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

int hash_add(tHashTable *tbl, char* key, void* value)
{
    check_if(tbl == NULL, return HASH_FAIL, "tbl is null");
    check_if(key == NULL, return HASH_FAIL, "key is null");
    check_if(value == NULL, return HASH_FAIL, "value is null");
    check_if(tbl->is_init != 1, return HASH_FAIL, "tbl is not init yet");

    int num = list_length(&(tbl->keys));
    check_if(num >= tbl->max_num, return HASH_FAIL, "tbl num = %d is full", num);

    int contain = hash_contains(tbl, key);
    check_if(contain, return HASH_FAIL, "key = %s is already added", key);

    int hret = _hadd(&(tbl->hdata), key, value);
    check_if(hret != HASH_OK, return HASH_FAIL, "_hadd failed");

    char* newkey = strdup(key);
    check_if(newkey == NULL, goto _ERROR, "strdup failed");

    int lret = list_append(&(tbl->keys), newkey);
    check_if(lret != LIST_OK, goto _ERROR, "list_append failed");

    return HASH_OK;

_ERROR:
    if (newkey) free(newkey);

    _hdelete(&(tbl->hdata), key);

    return HASH_FAIL;
}

int hash_modify(tHashTable* tbl, char* key, void* value)
{
    check_if(tbl == NULL, return HASH_FAIL, "tbl is null");
    check_if(key == NULL, return HASH_FAIL, "key is null");
    check_if(value == NULL, return HASH_FAIL, "value is null");
    check_if(tbl->is_init != 1, return HASH_FAIL, "tbl is not init yet");

    int contain = hash_contains(tbl, key);
    check_if(contain == 0, return HASH_FAIL, "key = %s not exist in table", key);

    return _hadd(&(tbl->hdata), key, value);
}

int hash_del(tHashTable* tbl, char* key, void** pvalue)
{
    check_if(tbl == NULL, return HASH_FAIL, "tbl is null");
    check_if(key == NULL, return HASH_FAIL, "key is null");
    check_if(pvalue == NULL, return HASH_FAIL, "pvalue is null");
    check_if(tbl->is_init != 1, return HASH_FAIL, "tbl is not init yet");

    char* newkey = list_find(&(tbl->keys), _comparekey, key);
    check_if(newkey == NULL, return HASH_FAIL, "key = %s not exist in keys list", key);

    list_remove(&(tbl->keys), newkey);
    free(newkey);

    *pvalue = _hfind(&(tbl->hdata), key);
    check_if(*pvalue == NULL, return HASH_FAIL, "key = %s not exist in table", key);

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
