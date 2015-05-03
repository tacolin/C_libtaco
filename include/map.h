#ifndef _MAP_H_
#define _MAP_H_

#include "basic.h"
#include "frwlock.h"

////////////////////////////////////////////////////////////////////////////////

#define MAP_OK (0)
#define MAP_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef void (*tMapContentCleanFn)(void* content);

typedef struct tMapSlot
{
    unsigned int id;
    int ref;
    void* content;

} tMapSlot;

typedef struct tMap
{
    unsigned int lastid;
    // tRwlock rwlock;
    tFrwlock rwlock;
    int max_num;
    int num;

    tMapSlot* slots;
    tMapContentCleanFn cleanfn;

    int is_init;

} tMap;

////////////////////////////////////////////////////////////////////////////////

int map_init(tMap* map, tMapContentCleanFn cleanfn);
int map_uninit(tMap* map);

int map_add(tMap* map, void* content, unsigned int* pid);

void* map_grab(tMap* map, unsigned int id);
void* map_release(tMap* map, unsigned int id);

int map_ids(tMap* map, int buf_size, unsigned int* id_buf);

#endif //_MAP_H_
