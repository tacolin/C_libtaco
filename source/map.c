#include "map.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#define MAP_INIT_MAX_SLOTS (16) // must be pow of 2

////////////////////////////////////////////////////////////////////////////////

#define GET_SLOT(map, id) &(map->slots[id & (map->max_num - 1)])

// #define INC_REF(ref) (ref)++
// #define DEC_REF(ref) (ref)--
#define INC_REF(ref) __sync_add_and_fetch(&ref, 1)
#define DEC_REF(ref) __sync_sub_and_fetch(&ref, 1)


////////////////////////////////////////////////////////////////////////////////

int _expandMap(tMap* map)
{
    check_if(map == NULL, return MAP_FAIL, "map is null");
    check_if(map->is_init != 1, return MAP_FAIL, "map is not init yet");

    int new_max_num = map->max_num * 2;
    tMapSlot* new_slots = calloc(sizeof(tMapSlot), new_max_num);
    check_if(new_slots == NULL, return MAP_FAIL, "calloc failed");

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
    map->slots   = new_slots;
    map->max_num = new_max_num;

    return MAP_OK;
}



////////////////////////////////////////////////////////////////////////////////

int map_init(tMap* map, tMapContentCleanFn cleanfn)
{
    check_if(map == NULL, return MAP_FAIL, "map is null");

    memset(map, 0, sizeof(tMap));

    frwlock_init(&map->rwlock);

    map->lastid  = 0;
    map->max_num = MAP_INIT_MAX_SLOTS;
    map->num     = 0;
    map->cleanfn = cleanfn;

    map->slots = calloc(sizeof(tMapSlot), map->max_num);
    check_if(map->slots == NULL, return MAP_FAIL, "calloc failed");

    map->is_init = 1;

    return MAP_OK;
}

int map_uninit(tMap* map)
{
    check_if(map == NULL, return MAP_FAIL, "map is null");
    check_if(map->is_init != 1, return MAP_FAIL, "map is not init yet");

    map->is_init = 0;

    frwlock_wlock(&map->rwlock);

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
    map->num     = 0;
    map->cleanfn = NULL;
    map->lastid = 0;

    frwlock_wunlock(&map->rwlock);

    return MAP_OK;
}

int map_add(tMap* map, void* content, unsigned int* pid)
{
    check_if(map == NULL, return MAP_FAIL, "map is null");
    check_if(content == NULL, return MAP_FAIL, "content is null");
    check_if(map->is_init != 1, return MAP_FAIL, "map is not init yet");

    frwlock_wlock(&map->rwlock);

    if (map->num >= (map->max_num * 3 / 4))
    {
        int ret = _expandMap(map);
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
            slot->id      = tmpid;
            slot->ref     = 1;
            slot->content = content;
            map->num++;

            *pid = tmpid;

            frwlock_wunlock(&map->rwlock);
            return MAP_OK;
        }

        // next one
    }

    derror("no slot is empty, but it is impossible");

_ERROR:
    frwlock_wunlock(&map->rwlock);
    return MAP_FAIL;
}

static void* _tryDelete(tMap* map, unsigned int id)
{
    check_if(map == NULL, return NULL, "map is null");
    check_if(map->is_init != 1, return NULL, "map is not init yet");
    check_if(id == 0, return NULL, "id = %d invalid", id);

    frwlock_wlock(&map->rwlock);

    tMapSlot* slot = GET_SLOT(map, id);
    check_if(slot->id != id, goto _ERROR, "id = %d not exist in map", id);
    check_if(slot->ref > 0, goto _ERROR, "id = %d slot ref = %d", id , slot->ref);

    void* content = slot->content;

    slot->content = NULL;
    slot->id      = 0;
    slot->ref     = 0;
    map->num--;

    frwlock_wunlock(&map->rwlock);
    return content;

_ERROR:
    frwlock_wunlock(&map->rwlock);
    return NULL;
}

void* map_grab(tMap* map, unsigned int id)
{
    check_if(map == NULL, return NULL, "map is null");
    check_if(map->is_init != 1, return NULL, "map is not init yet");
    check_if(id == 0, return NULL, "id = %d invalid", id);

    void* ret = NULL;

    frwlock_rlock(&map->rwlock);

    tMapSlot* slot = GET_SLOT(map, id);
    check_if(slot->id != id, goto _END, "id = %d not exist in map", id);

    INC_REF(slot->ref);

    ret = slot->content;

_END:
    frwlock_runlock(&map->rwlock);
    return ret;
}

static void* _releaseRef(tMap* map, unsigned int id)
{
    check_if(map == NULL, return NULL, "map is null");
    check_if(map->is_init != 1, return NULL, "map is not init yet");
    check_if(id == 0, return NULL, "id = %d invalid", id);

    frwlock_rlock(&map->rwlock);

    tMapSlot* slot = GET_SLOT(map, id);
    check_if(slot->id != id, goto _ERROR, "id = %d not exist in map", id);

    void* content;
    if (DEC_REF(slot->ref) <= 0)
    {
        content = slot->content;
    }
    else
    {
        content = NULL;
    }

    frwlock_runlock(&map->rwlock);
    return content;

_ERROR:
    frwlock_runlock(&map->rwlock);
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

// int map_list(tMap* map, int num, unsigned int* results)
int map_ids(tMap* map, int buf_size, unsigned int* id_buf)
{
    check_if(map == NULL, return -1, "map is null");
    check_if(map->is_init != 1, return -1, "map is not init yet");
    check_if(buf_size == 0, return -1, "buf_size = %d invalid", buf_size);
    check_if(id_buf == NULL, return -1, "id_buf is null");

    int i;
    int ret_num;

    frwlock_rlock(&map->rwlock);

    for (i=0; (ret_num<buf_size) && (i<map->max_num); i++)
    {
        tMapSlot* slot = &(map->slots[i]);
        if (slot->id != 0)
        {
            id_buf[ret_num] = slot->id;
            ret_num++;
        }
    }

    // ret_num = map->num;

    frwlock_runlock(&map->rwlock);

    return ret_num;
}
