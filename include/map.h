#ifndef _MAP_H_
#define _MAP_H_

#include "basic.h"
#include "lock.h"

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    MAP_OK = 0,
    MAP_ERROR = -1

} tMapStatus;

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
    tRwlock rwlock;
    int max_num;
    int num;

    tMapSlot* slots;
    tMapContentCleanFn cleanfn;

    int is_init;

} tMap;

////////////////////////////////////////////////////////////////////////////////

tMapStatus map_init(tMap* map, tMapContentCleanFn cleanfn);
tMapStatus map_uninit(tMap* map);

tMapStatus map_add(tMap* map, void* content, unsigned int* pid);

void* map_grab(tMap* map, unsigned int id);
void* map_release(tMap* map, unsigned int id);

#endif //_MAP_H_
