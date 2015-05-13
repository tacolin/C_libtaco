#ifndef _MAP_H_
#define _MAP_H_

#include "rwlock.h"

#define MAP_OK (0)
#define MAP_FAIL (-1)

#define MAPID_INVALID (0)

typedef unsigned int mapid;

struct map_slot
{
    mapid id;
    int   ref;
    void* data;
};

struct map
{
    mapid lastid;
    struct rwlock lock;

    int max_num;
    int num;

    struct map_slot* slots;
    void (*cleanfn)(void* data);

    int is_init;
};

int map_init(struct map* map, void (*cleanfn)(void*));
int map_uninit(struct map* map);

mapid map_new(struct map* map, void *data);

void* map_grab(struct map* map, mapid id);
void* map_release(struct map* map, mapid id);

int map_list(struct map* map, mapid* idbuf, int bufsize);

#endif //_MAP_H_
