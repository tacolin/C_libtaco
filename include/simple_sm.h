#ifndef _SIMPLE_SM_H_
#define _SIMPLE_SM_H_

#include "basic.h"
#include "tree.h"
#include "queue.h"
#include "hash.h"

#define SM_NAME_SIZE 100
#define SM_MAX_EV_NUM 1000

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    SM_OK = 0,
    SM_ERROR = -1

} tSmStatus;

struct tSmSt;
struct tSm;

typedef tSmStatus (*tSmStEntryFn)(struct tSm* sm, struct tSmSt* state);
typedef tSmStatus (*tSmStExitFn)(struct tSm* sm, struct tSmSt* state);
typedef tSmStatus (*tSmTransAction)(struct tSm* sm, void* ev_data);
typedef int (*tSmGuardFn)(struct tSm* sm, struct tSmSt* state, void* ev_data);

typedef struct tSmEv
{
    int evid;
    void* ev_data;

} tSmEv;

typedef struct tSmTrans
{
    int evid;

    tSmGuardFn on_guard;
    int guard_value;

    struct tSmSt* next_st;

    tList route;
    struct tSmSt* ancestor;

    tSmTransAction on_act;

} tSmTrans;

struct tSm;

typedef struct tSmSt
{
    tTreeHdr hdr;

    char name[SM_NAME_SIZE];

    tSmStEntryFn on_entry;
    tSmStExitFn  on_exit;

    tList translist;

    struct tSm* sm;

    int is_init_state;

} tSmSt;

typedef struct tSm
{
    char name[SM_NAME_SIZE];

    tTree st_tree;
    tSmSt* root;
    tSmSt* curr_st;

    tHashTable ev_hash;
    int last_ev_id;

    int is_init;
    int is_started;
    int is_busy;
    tQueue evqueue;

    void* db;

} tSm;

////////////////////////////////////////////////////////////////////////////////

tSmStatus sm_init(tSm* sm, char* name);
tSmStatus sm_uninit(tSm* sm);

tSmStatus sm_setDb(tSm* sm, void* db);

tSmSt* sm_rootSt(tSm* sm);

tSmSt* sm_createSt(tSm* sm, char* name, tSmStEntryFn on_entry, tSmStExitFn on_exit);
tSmStatus sm_addSt(tSm* sm, tSmSt* parent_st, tSmSt* child_st, int is_init_state);

tSmStatus sm_addEv(tSm* sm, char* ev_name);

tSmTrans* sm_createTrans(tSm* sm, char* ev_name, tSmGuardFn on_guard, int guard_value, char* next_st_name, tSmTransAction on_act);
tSmStatus sm_addTrans(tSm* sm, tSmSt* start_st, tSmTrans* trans);

tSmStatus sm_start(tSm* sm);
tSmStatus sm_stop(tSm* sm);

tSmStatus sm_sendEv(tSm* sm, char* ev_name, void* ev_data);

#endif //_SIMPLE_SM_H_
