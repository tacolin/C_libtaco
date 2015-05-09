#ifndef _FSM_H_
#define _FSM_H_

#include <stdbool.h>
#include "fast_queue.h"

#define FSM_OK (0)
#define FSM_FAIL (-1)

struct fsmev
{
    int type;
    void* data;
};

struct fsmst;
struct fsm;

struct fsmtrans
{
    int ev_type;
    void (*action)(struct fsm* fsm, void* db, struct fsmev* ev);
    int (*guard)(struct fsm* fsm, void* db, struct fsmev* ev);
    int guard_val;
    struct fsmst* next_st;
};

struct fsmst
{
    char* name;
    void (*entryfn)(struct fsm* fsm, void* db, struct fsmst* st);
    void (*exitfn)(struct fsm* fsm, void* db, struct fsmst* st);
    struct fsmtrans* trans_array;
};

struct fsm
{
    bool busy;
    struct fsmst* curr;
    struct fsmst* prev;
    struct fqueue* evqueue;
    void* db;
};

int fsm_init(struct fsm* fsm, struct fsmst* init_st, void* db);
void fsm_uninit(struct fsm* fsm);

int fsm_sendev(struct fsm* fsm, struct fsmev* ev);

#endif //_FSM_H_
