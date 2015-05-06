#include <stdio.h>
#include <string.h>

#include "hash.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static int _findkey(void* data, void* arg)
{
    CHECK_IF(data == NULL, return 0, "data is null");
    CHECK_IF(arg == NULL, return 0, "arg is null");
    return (strcmp((char*)data, (char*)arg) == 0) ? 1 : 0;
}

static int _hcreate(struct hsearch_data* hdata, int max_num)
{
    CHECK_IF(hdata == NULL, return HASH_FAIL, "hdata is null");
    CHECK_IF(max_num <= 0, return HASH_FAIL, "max_num = %d invalid", max_num);

    int chk = hcreate_r(max_num, hdata);
    CHECK_IF(chk == 0, return HASH_FAIL, "hcreate_r failed");
    return HASH_OK;
}

static void _hdestroy(struct hsearch_data* hdata)
{
    CHECK_IF(hdata == NULL, return, "hdata is null");
    hdestroy_r(hdata);
}

static int _hadd(struct hsearch_data* hdata, char* key, void* value)
{
    CHECK_IF(hdata == NULL, return HASH_FAIL, "hdata is null");
    CHECK_IF(key == NULL, return HASH_FAIL, "key is null");
    CHECK_IF(value == NULL, return HASH_FAIL, "value is null");

    ENTRY item = {
        .key = key,
        .data = value
    };
    ENTRY *pitem = NULL;
    int chk = hsearch_r(item, ENTER, &pitem, hdata);
    CHECK_IF(chk == 0, return HASH_FAIL, "hsearch_r ENTER failed");

    pitem->data = value;
    return HASH_OK;
}

static int _hdelete(struct hsearch_data* hdata, char* key)
{
    CHECK_IF(hdata == NULL, return HASH_FAIL, "hdata is null");
    CHECK_IF(key == NULL, return HASH_FAIL, "key is null");

    ENTRY item = {
        .key = key
    };
    ENTRY *pitem = NULL;
    int chk = hsearch_r(item, FIND, &pitem, hdata);
    CHECK_IF(chk == 0, return HASH_FAIL, "hsearch_r FIND failed");

    pitem->data = NULL;
    return HASH_OK;
}

static void* _hfind(struct hsearch_data* hdata, char* key)
{
    CHECK_IF(hdata == NULL, return NULL, "hdata is null");
    CHECK_IF(key == NULL, return NULL, "key is null");

    ENTRY item = {
        .key = key
    };
    ENTRY *pitem = NULL;
    int chk = hsearch_r(item, FIND, &pitem, hdata);
    if (chk == 0) return NULL;
    if (pitem == NULL) return NULL;

    return pitem->data;
}

int hash_init(struct hash* h, int max_num, void (*cleanfn)(void*))
{
    CHECK_IF(h == NULL, return HASH_FAIL, "h is null");

    memset(h, 0, sizeof(struct hash));

    int chk = list_init(&(h->keys), free);
    CHECK_IF(chk != LIST_OK, return HASH_FAIL, "list_init failed");

    h->max_num = max_num;
    h->cleanfn = cleanfn;

    chk = _hcreate(&(h->hdata), max_num);
    CHECK_IF(chk != HASH_OK, return HASH_FAIL, "_hcreate failed");

    h->is_init = 1;
    return HASH_OK;
}

int hash_uninit(struct hash* h)
{
    CHECK_IF(h == NULL, return HASH_FAIL, "h is null");
    CHECK_IF(h->is_init != 1, return HASH_FAIL, "h is not init yet");

    if (h->cleanfn)
    {
        char* key;
        void* value;
        HASH_FOREACH(h, key, value)
        {
            h->cleanfn(value);
        }
    }
    list_clean(&(h->keys));
    _hdestroy(&(h->hdata));
    return HASH_OK;
}

int hash_add(struct hash *h, char* key, void* value)
{
    CHECK_IF(h == NULL, return HASH_FAIL, "h is null");
    CHECK_IF(key == NULL, return HASH_FAIL, "key is null");
    CHECK_IF(value == NULL, return HASH_FAIL, "value is null");
    CHECK_IF(h->is_init != 1, return HASH_FAIL, "h is not init yet");
    CHECK_IF(h->max_num <= list_num(&h->keys), return HASH_FAIL, "h is full");

    int contain = hash_contains(h, key);
    CHECK_IF(contain, return HASH_FAIL, "key = %s is already added", key);

    int chk = _hadd(&(h->hdata), key, value);
    CHECK_IF(chk != HASH_OK, return HASH_FAIL, "_hadd failed");

    char* newkey = strdup(key);
    CHECK_IF(newkey == NULL, goto _ERROR, "strdup failed");

    chk = list_append(&(h->keys), newkey);
    CHECK_IF(chk != LIST_OK, goto _ERROR, "list_append failed");

    return HASH_OK;

_ERROR:
    if (newkey) free(newkey);

    _hdelete(&(h->hdata), key);
    return HASH_FAIL;
}

int hash_modify(struct hash* h, char* key, void* value)
{
    CHECK_IF(h == NULL, return HASH_FAIL, "h is null");
    CHECK_IF(key == NULL, return HASH_FAIL, "key is null");
    CHECK_IF(value == NULL, return HASH_FAIL, "value is null");
    CHECK_IF(h->is_init != 1, return HASH_FAIL, "h is not init yet");

    int contain = hash_contains(h, key);
    CHECK_IF(contain == 0, return HASH_FAIL, "key = %s not exist in table", key);
    return _hadd(&(h->hdata), key, value);
}

int hash_del(struct hash* h, char* key, void** pvalue)
{
    CHECK_IF(h == NULL, return HASH_FAIL, "h is null");
    CHECK_IF(key == NULL, return HASH_FAIL, "key is null");
    CHECK_IF(pvalue == NULL, return HASH_FAIL, "pvalue is null");
    CHECK_IF(h->is_init != 1, return HASH_FAIL, "h is not init yet");

    char* newkey = list_find(&(h->keys), _findkey, key);
    CHECK_IF(newkey == NULL, return HASH_FAIL, "key = %s not exist in keys list", key);

    list_remove(&(h->keys), newkey);
    free(newkey);

    *pvalue = _hfind(&(h->hdata), key);
    CHECK_IF(*pvalue == NULL, return HASH_FAIL, "key = %s not exist in table", key);
    return _hdelete(&(h->hdata), key);
}

void* hash_find(struct hash *h, char* key)
{
    CHECK_IF(h == NULL, return NULL, "h is null");
    // CHECK_IF(key == NULL, return NULL, "key is null");
    CHECK_IF(h->is_init != 1, return NULL, "h is not init yet");

    if (key == NULL) return NULL;

    return _hfind(&(h->hdata), key);
}

int   hash_contains(struct hash *h, char* key)
{
    CHECK_IF(h == NULL, return 0, "h is null");
    CHECK_IF(key == NULL, return 0, "key is null");
    CHECK_IF(h->is_init != 1, return 0, "h is not init yet");
    return (NULL != _hfind(&(h->hdata), key)) ? 1 : 0;
}

char* hash_first_key(struct hash* h)
{
    CHECK_IF(h == NULL, return 0, "h is null");
    CHECK_IF(h->is_init != 1, return 0, "h is not init yet");
    return list_head(&(h->keys));
}

char* hash_next_key(struct hash* h, char* newkey)
{
    CHECK_IF(h == NULL, return 0, "h is null");
    CHECK_IF(newkey == NULL, return 0, "newkey is null");
    CHECK_IF(h->is_init != 1, return 0, "h is not init yet");
    return list_next(&(h->keys), newkey);
}
