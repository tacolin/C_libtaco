#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"

#define MAP_INIT_MAX_SLOTS (16) // must be pow of 2

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#define GET_SLOT(map, id) &(map->slots[id & (map->max_num - 1)])
#define INC_REF(ref) __sync_add_and_fetch(&ref, 1)
#define DEC_REF(ref) __sync_sub_and_fetch(&ref, 1)

static int _expand_map(struct map* map)
{
    CHECK_IF(map == NULL, return MAP_FAIL, "map is null");
    CHECK_IF(map->is_init != 1, return MAP_FAIL, "map is not init yet");

    int new_max_num = map->max_num * 2;
    struct map_slot* new_slots = calloc(sizeof(struct map_slot), new_max_num);
    CHECK_IF(new_slots == NULL, return MAP_FAIL, "calloc failed");

    int i;
    struct map_slot* os;
    struct map_slot* ns;
    for (i=0; i<map->max_num; i++)
    {
        os = &(map->slots[i]);
        ns = &(new_slots[os->id & (new_max_num -1)]);
        *ns = *os;
    }
    free(map->slots);
    map->slots   = new_slots;
    map->max_num = new_max_num;
    return MAP_OK;
}

int map_init(struct map* map, void (*cleanfn)(void*))
{
    CHECK_IF(map == NULL, return MAP_FAIL, "map is null");

    memset(map, 0, sizeof(struct map));
    rwlock_init(&map->lock);
    map->lastid  = 0;
    map->max_num = MAP_INIT_MAX_SLOTS;
    map->num     = 0;
    map->cleanfn = cleanfn;
    map->slots   = calloc(sizeof(struct map_slot), map->max_num);
    CHECK_IF(map->slots == NULL, return MAP_FAIL, "calloc failed");

    map->is_init = 1;
    return MAP_OK;
}

int map_uninit(struct map* map)
{
    CHECK_IF(map == NULL, return MAP_FAIL, "map is null");
    CHECK_IF(map->is_init != 1, return MAP_FAIL, "map is not init yet");

    map->is_init = 0;

    rwlock_wlock(&map->lock);
    if (map->cleanfn)
    {
        struct map_slot* slot;
        int i;
        for (i=0; i<map->max_num; i++)
        {
            slot = &(map->slots[i]);
            if ((slot->id != 0) && slot->data)
            {
                map->cleanfn(slot->data);
            }
        }
    }
    free(map->slots);
    map->slots = NULL;
    map->max_num = 0;
    map->num     = 0;
    map->cleanfn = NULL;
    map->lastid = 0;
    rwlock_wunlock(&map->lock);
    return MAP_OK;
}

mapid map_new(struct map* map, void* data)
{
    CHECK_IF(map == NULL, return MAPID_INVALID, "map is null");
    CHECK_IF(data == NULL, return MAPID_INVALID, "data is null");
    CHECK_IF(map->is_init != 1, return MAPID_INVALID, "map is not init yet");

    rwlock_wlock(&map->lock);

    if (map->num >= (map->max_num * 3 / 4))
    {
        int chk = _expand_map(map);
        CHECK_IF(chk != MAP_OK, goto _ERROR, "_expand_map failed");
    }

    int i;
    struct map_slot* slot;
    mapid tmpid;
    for (i=0; i<map->max_num; i++)
    {
        map->lastid++;
        tmpid = map->lastid;

        if (tmpid == MAPID_INVALID)
        {
            map->lastid++;
            tmpid = map->lastid;
        }

        slot = GET_SLOT(map, tmpid);
        if (slot->id == MAPID_INVALID)
        {
            // find a empty slot
            slot->id   = tmpid;
            slot->ref  = 1;
            slot->data = data;
            map->num++;
            rwlock_wunlock(&map->lock);
            return tmpid;
        }

        // next one
    }

    derror("no slot is empty, but it is impossible");

_ERROR:
    rwlock_wunlock(&map->lock);
    return MAPID_INVALID;
}

static void* _try_delete(struct map* map, mapid id)
{
    CHECK_IF(map == NULL, return NULL, "map is null");
    CHECK_IF(map->is_init != 1, return NULL, "map is not init yet");
    CHECK_IF(id == MAPID_INVALID, return NULL, "id = %d invalid", id);

    rwlock_wlock(&map->lock);

    struct map_slot* slot = GET_SLOT(map, id);
    CHECK_IF(slot->id != id, goto _ERROR, "id = %d not exist in map", id);
    CHECK_IF(slot->ref > 0, goto _ERROR, "id = %d slot ref = %d", id , slot->ref);

    void* data = slot->data;
    slot->data = NULL;
    slot->id   = 0;
    slot->ref  = 0;
    map->num--;

    rwlock_wunlock(&map->lock);
    return data;

_ERROR:
    rwlock_wunlock(&map->lock);
    return NULL;
}

void* map_grab(struct map* map, mapid id)
{
    CHECK_IF(map == NULL, return NULL, "map is null");
    CHECK_IF(map->is_init != 1, return NULL, "map is not init yet");
    CHECK_IF(id == MAPID_INVALID, return NULL, "id = %d invalid", id);

    void* ret = NULL;

    rwlock_rlock(&map->lock);

    struct map_slot* slot = GET_SLOT(map, id);
    if (slot->id != id) goto _END;
    // CHECK_IF(slot->id != id, goto _END, "id = %d not exist in map", id);

    INC_REF(slot->ref);

    ret = slot->data;

_END:
    rwlock_runlock(&map->lock);
    return ret;
}

static void* _releaseRef(struct map* map, mapid id)
{
    CHECK_IF(map == NULL, return NULL, "map is null");
    CHECK_IF(map->is_init != 1, return NULL, "map is not init yet");
    CHECK_IF(id == MAPID_INVALID, return NULL, "id = %d invalid", id);

    rwlock_rlock(&map->lock);

    struct map_slot* slot = GET_SLOT(map, id);
    if (slot->id != id) goto _ERROR;
    // CHECK_IF(slot->id != id, goto _ERROR, "id = %d not exist in map", id);

    void* data;
    if (DEC_REF(slot->ref) <= 0)
    {
        data = slot->data;
    }
    else
    {
        data = NULL;
    }

    rwlock_runlock(&map->lock);
    return data;

_ERROR:
    rwlock_runlock(&map->lock);
    return NULL;
}

void* map_release(struct map* map, mapid id)
{
    CHECK_IF(map == NULL, return NULL, "map is null");
    CHECK_IF(map->is_init != 1, return NULL, "map is not init yet");
    CHECK_IF(id == MAPID_INVALID, return NULL, "id = %d invalid", id);

    if (NULL == _releaseRef(map, id))
    {
        return NULL;
    }
    return _try_delete(map, id);
}
