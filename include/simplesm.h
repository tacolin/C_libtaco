#ifndef _SIMPLE_SM_H_
#define _SIMPLE_SM_H_

#include "basic.h"
#include "queue.h"

////////////////////////////////////////////////////////////////////////////////

#define EV_NAME_SIZE 30
#define QUEUE_EV_NUM 100

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    SM_OK = 0,
    SM_ERROR = -1

} tSmStatus;

////////////////////////////////////////////////////////////////////////////////

struct tSmSt;

typedef tSmStatus (*tSmAction)(struct tSmSt* start, void* ev_data);
typedef tSmStatus (*tSmEntryStFn)(struct tSmSt* state);
typedef tSmStatus (*tSmExitStFn)(struct tSmSt* state);

typedef void (*tSmEvDataFreeFn)(void* ev_data);

typedef int (*tSmGuard)(struct tSmSt* start, void* ev_data);

typedef struct tSmEv
{
    char ev_name[EV_NAME_SIZE];
    void* ev_data;
    tSmEvDataFreeFn fn;

} tSmEv;

typedef struct tSmTrans
{
    char* ev_name;
    tSmGuard guard;
    int guard_value;
    char* next_st_name;
    tSmAction action;

} tSmTrans;

typedef struct tSmSt
{
    char* name;

    tSmEntryStFn on_entry;
    tSmExitStFn  on_exit;

    int is_init_state;

    struct tSmSt* sub_states;

    tSmTrans* transitions;

    void* db;

    ////////////////////////////////////////////////////////////////
    // the following are run-timer variables, no need to set
    ////////////////////////////////////////////////////////////////
    int sub_state_num;
    int transition_num;

    tQueue evqueue;

    int is_completed;
    int is_started;
    int is_busy;
    struct tSmSt* curr_sub_state;

} tSmSt;

////////////////////////////////////////////////////////////////////////////////

tSmStatus sm_complete(tSmSt* root, void* db, tSmEvDataFreeFn fn);
tSmStatus sm_start(tSmSt* root);
tSmStatus sm_stop(tSmSt* root);
tSmStatus sm_sendev(tSmSt* root, char* ev_name, void* ev_data);

#endif //_SIMPLE_SM_H_
