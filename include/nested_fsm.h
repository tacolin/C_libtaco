#ifndef _NESTED_FSM_H_
#define _NESTED_FSM_H_

#include <stdbool.h>
#include "fast_queue.h"

#define NFSM_OK (0)
#define NFSM_FAIL (-1)

struct nfsmev
{
    int type;
    void* data;
};

struct nfsmst;
struct nfsm;

struct nfsmtrans
{
    int ev_type;
    void (*action)(struct nfsm* nfsm, void* db, struct nfsmev* ev);
    int (*guard)(struct nfsm* nfsm, void* db, struct nfsmev* ev);
    int guard_val;
    struct nfsmst* next_st;
};

struct nfsmst
{
    char* name;
    struct nfsmst* parent;
    struct nfsmst* init_subst;
    struct nfsmtrans* trans_array;
};

struct nfsm
{
    bool busy;
    struct nfsmst* curr;
    struct nfsmst* prev;
    struct fqueue* evqueue;
    void* db;
};

int nfsm_init(struct nfsm* nfsm, struct nfsmst* init_st, void* db);
void nfsm_uninit(struct nfsm* nfsm);

int nfsm_sendev(struct nfsm* nfsm, struct nfsmev* ev);
struct nfsmst* nfsm_curr(struct nfsm* nfsm);

#endif //_NESTED_FSM_H_
