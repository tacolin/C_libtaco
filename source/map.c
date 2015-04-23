#include "map.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#define MAP_INIT_MAX_SLOTS (16) // must be pow of 2

////////////////////////////////////////////////////////////////////////////////

#define GET_SLOT(map, id) &(map->slots[id & (map->max_num - 1)])

#define INC_REF(ref) (ref)++
#define DEC_REF(ref) (ref)--
// #define INC_REF(ref) __sync_add_and_fetch(&ref, 1)
// #define DEC_REF(ref) __sync_sub_and_fetch(&ref, 1)


////////////////////////////////////////////////////////////////////////////////

tMapStatus _expandMap(tMap* map)
{
    check_if(map == NULL, return MAP_ERROR, "map is null");
    check_if(map->is_init != 1, return MAP_ERROR, "map is not init yet");

    int new_max_num = map->max_num * 2;
    tMapSlot* new_slots = calloc(sizeof(tMapSlot), new_max_num);
    check_if(new_slots == NULL, return MAP_ERROR, "calloc failed");

    int i;
    tMapSlot* old_one;
    tMapSlot* new_one;
    for (i=0; i<map->max_num; i++)
    {
        old_one = &(map->slots[i]);
        new_one = &(new_slots[old_one->id & (new_max_num -1)]);
        *new_one = *old_one;
    }

    free(map->slots);
    map->slots = new_slots;
    map->max_num = new_max_num;

    return MAP_OK;
}



////////////////////////////////////////////////////////////////////////////////

tMapStatus map_init(tMap* map, tMapContentCleanFn cleanfn)
{
    check_if(map == NULL, return MAP_ERROR, "map is null");

    memset(map, 0, sizeof(tMap));

    tLockStatus lock_ret = rwlock_init(&map->rwlock);
    check_if(lock_ret != LOCK_OK, return MAP_ERROR, "rwlock_init failed");

    map->lastid = 0;
    map->max_num = MAP_INIT_MAX_SLOTS;
    map->num = 0;
    map->cleanfn = cleanfn;

    map->slots = calloc(sizeof(tMapSlot), map->max_num);
    check_if(map->slots == NULL, return MAP_ERROR, "calloc failed");

    map->is_init = 1;

    return MAP_OK;
}

tMapStatus map_uninit(tMap* map)
{
    check_if(map == NULL, return MAP_ERROR, "map is null");
    check_if(map->is_init != 1, return MAP_ERROR, "map is not init yet");

    map->is_init = 0;

    rwlock_enterWrite(&map->rwlock);

    if (map->cleanfn)
    {
        tMapSlot* slot;
        int i;
        for (i=0; i<map->max_num; i++)
        {
            slot = &(map->slots[i]);
            if ((slot->id != 0) && slot->content)
            {
                map->cleanfn(slot->content);
            }
        }
    }

    free(map->slots);
    map->slots = NULL;

    map->max_num = 0;
    map->num = 0;
    map->cleanfn = NULL;
    map->lastid = 0;

    rwlock_exitWrite(&map->rwlock);

    rwlock_uninit(&map->rwlock);

    return MAP_OK;
}

tMapStatus map_add(tMap* map, void* content, unsigned int* pid)
{
    check_if(map == NULL, return MAP_ERROR, "map is null");
    check_if(content == NULL, return MAP_ERROR, "content is null");
    check_if(map->is_init != 1, return MAP_ERROR, "map is not init yet");

    rwlock_enterWrite(&map->rwlock);

    if (map->num >= (map->max_num * 3 / 4))
    {
        tMapStatus ret = _expandMap(map);
        check_if(ret != MAP_OK, goto _ERROR, "_expandMap failed");
    }

    int i;
    tMapSlot* slot;
    unsigned int tmpid;
    for (i=0; i<map->max_num; i++)
    {
        map->lastid++;
        tmpid = map->lastid;

        if (tmpid == 0)
        {
            map->lastid++;
            tmpid = map->lastid;
        }

        slot = GET_SLOT(map, tmpid);
        if (slot->id == 0)
        {
            // find a empty slot
            slot->id = tmpid;
            slot->ref = 1;
            slot->content = content;
            map->num++;

            *pid = tmpid;

            rwlock_exitWrite(&map->rwlock);
            return MAP_OK;
        }

        // next one
    }

    derror("no slot is empty, but it is impossible");

_ERROR:
    rwlock_exitWrite(&map->rwlock);
    return MAP_ERROR;
}

static void* _tryDelete(tMap* map, unsigned int id)
{
    check_if(map == NULL, return NULL, "map is null");
    check_if(map->is_init != 1, return NULL, "map is not init yet");
    check_if(id == 0, return NULL, "id = %d invalid", id);

    rwlock_enterWrite(&map->rwlock);

    tMapSlot* slot = GET_SLOT(map, id);
    check_if(slot->id != id, goto _ERROR, "id = %d not exist in map", id);
    check_if(slot->ref > 0, goto _ERROR, "id = %d slot ref = %d", id , slot->ref);

    void* content = slot->content;

    slot->content = NULL;
    slot->id = 0;
    slot->ref = 0;
    map->num--;

    rwlock_exitWrite(&map->rwlock);
    return content;

_ERROR:
    rwlock_exitWrite(&map->rwlock);
    return NULL;
}

void* map_grab(tMap* map, unsigned int id)
{
    check_if(map == NULL, return NULL, "map is null");
    check_if(map->is_init != 1, return NULL, "map is not init yet");
    check_if(id == 0, return NULL, "id = %d invalid", id);

    void* ret = NULL;

    rwlock_enterRead(&map->rwlock);

    tMapSlot* slot = GET_SLOT(map, id);
    check_if(slot->id != id, goto _END, "id = %d not exist in map", id);

    INC_REF(slot->ref);

    ret = slot->content;

_END:
    rwlock_exitRead(&map->rwlock);
    return ret;
}

static void* _releaseRef(tMap* map, unsigned int id)
{
    check_if(map == NULL, return NULL, "map is null");
    check_if(map->is_init != 1, return NULL, "map is not init yet");
    check_if(id == 0, return NULL, "id = %d invalid", id);

    rwlock_enterRead(&map->rwlock);

    tMapSlot* slot = GET_SLOT(map, id);
    check_if(slot->id != id, goto _ERROR, "id = %d not exist in map", id);

    if (slot->ref > 0) DEC_REF(slot->ref);

    void* content = slot->content;

    rwlock_exitRead(&map->rwlock);
    return content;

_ERROR:
    rwlock_exitRead(&map->rwlock);
    return NULL;
}

void* map_release(tMap* map, unsigned int id)
{
    check_if(map == NULL, return NULL, "map is null");
    check_if(map->is_init != 1, return NULL, "map is not init yet");
    check_if(id == 0, return NULL, "id = %d invalid", id);

    if (NULL == _releaseRef(map, id))
    {
        return NULL;
    }

    return _tryDelete(map, id);
}
