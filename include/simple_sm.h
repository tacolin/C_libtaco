#ifndef _SIMPLE_SM_H_
#define _SIMPLE_SM_H_

#include "basic.h"
#include "tree.h"
#include "queue.h"

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////

#define SM_OK (0)
#define SM_FAIL (-1)

#define SM_NAME_SIZE 100
#define SM_MAX_EVQUEUE_NUM 100

////////////////////////////////////////////////////////////////////////////////

struct tSmEv;
struct tSmSt;
struct tSm;

typedef int (*tSmStEntryFn)(struct tSm* sm, struct tSmSt* state);
typedef int (*tSmStExitFn)(struct tSm* sm, struct tSmSt* state);
typedef int (*tSmTransAction)(struct tSm* sm, struct tSmEv* ev);
typedef int8_t (*tSmGuardFn)(struct tSm* sm, struct tSmSt* state, struct tSmEv* ev);

typedef struct tSmEv
{
    int evid;
    intptr_t arg1;
    intptr_t arg2;

} tSmEv;

typedef struct tSmTrans
{
    int evid;

    tSmGuardFn on_guard;
    int8_t guard_value;

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

    // tList translist;

    tSmGuardFn* guard_array;
    tSmTrans**  trans_array;

    struct tSmSt* curr_sub_state;

    struct tSm* sm;

    int is_init_state;

} tSmSt;

typedef struct tSm
{
    char name[SM_NAME_SIZE];

    tTree st_tree;
    tSmSt* root;

    int max_ev_value;
    int ev_bits;

    int is_init;
    int is_started;
    int is_busy;
    tQueue evqueue;

    void* db;

} tSm;

////////////////////////////////////////////////////////////////////////////////

int sm_init(tSm* sm, char* name, int max_ev_value);
int sm_uninit(tSm* sm);

int sm_setDb(tSm* sm, void* db);

tSmSt* sm_rootSt(tSm* sm);

tSmSt* sm_createSt(tSm* sm, char* name, tSmStEntryFn on_entry, tSmStExitFn on_exit);
int sm_addSt(tSm* sm, tSmSt* parent_st, tSmSt* child_st, int is_init_state);

tSmTrans* sm_createTrans(tSm* sm, int evid, tSmGuardFn on_guard, int8_t guard_value, char* next_st_name, tSmTransAction on_act);
int sm_addTrans(tSm* sm, tSmSt* start_st, tSmTrans* trans);

int sm_start(tSm* sm);
int sm_stop(tSm* sm);

int sm_sendEv(tSm* sm, int evid, intptr_t arg1, intptr_t arg2);

#endif //_SIMPLE_SM_H_
