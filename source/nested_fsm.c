#include <stdio.h>
#include <stdlib.h>
#include "nested_fsm.h"

#define INVALID_EV_TYPE (-1)

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static void _clean_nfsmev(void* input)
{
    if (input) free(input);
}

int nfsm_init(struct nfsm* nfsm, struct nfsmst* init_st, void* db)
{
    CHECK_IF(nfsm == NULL, return NFSM_FAIL, "nfsm is null");
    CHECK_IF(init_st == NULL, return NFSM_FAIL, "init_st is null");

    while (init_st->init_subst)
    {
        init_st = init_st->init_subst;
    }
    nfsm->curr    = init_st;
    nfsm->prev    = NULL;
    nfsm->busy    = false;
    nfsm->db      = db;
    nfsm->evqueue = fqueue_create(_clean_nfsmev);
    CHECK_IF(nfsm->evqueue == NULL, return NFSM_FAIL, "fqueue_create failed");
    return NFSM_OK;
}

void nfsm_uninit(struct nfsm* nfsm)
{
    CHECK_IF(nfsm == NULL, return, "nfsm is null");
    fqueue_release(nfsm->evqueue);
    nfsm->evqueue = NULL;
    nfsm->prev    = NULL;
    nfsm->curr    = NULL;
    nfsm->busy    = false;
    return;
}

struct nfsmret
{
    bool found;
    struct nfsmst* st;
    struct nfsmtrans* trans;
};

static struct nfsmret _find_st_and_trans(struct nfsm* nfsm, struct nfsmst* st, struct nfsmev* ev)
{
    struct nfsmret ret = {.found = false};
    CHECK_IF(nfsm == NULL, return ret, "nfsm is null");
    CHECK_IF(st == NULL, return ret, "st is null");
    CHECK_IF(ev == NULL, return ret, "ev is null");

    if (st->parent)
    {
        ret = _find_st_and_trans(nfsm, st->parent, ev);
        if (ret.found) return ret;
    }

    int i;
    for (i=0; st->trans_array[i].ev_type != INVALID_EV_TYPE; i++)
    {
        struct nfsmtrans* trans = &(st->trans_array[i]);
        if (trans->ev_type == ev->type)
        {
            if (trans->guard == NULL)
            {
                ret.found = true;
                ret.st    = st;
                ret.trans = trans;
                return ret;
            }
            else if (trans->guard(nfsm, nfsm->db, ev) == trans->guard_val)
            {
                ret.found = true;
                ret.st    = st;
                ret.trans = trans;
                return ret;
            }

        }
    }
    ret.found = false;
    return ret;
}

static void _handle_ev(struct nfsm* nfsm, struct nfsmev* ev)
{
    struct nfsmst* curr = nfsm->curr;
    struct nfsmtrans* trans;
    struct nfsmret result = _find_st_and_trans(nfsm, curr, ev);
    if (result.found == false) return;

    curr  = result.st;
    trans = result.trans;
    struct nfsmst* next = trans->next_st;
    if (next)
    {
        while (next->init_subst) next = next->init_subst;
    }
    else
    {
        next = curr;
    }

    if (trans->action) trans->action(nfsm, nfsm->db, ev);

    nfsm->prev = curr;
    nfsm->curr = next;
    return;
}

int nfsm_sendev(struct nfsm* nfsm, struct nfsmev* ev)
{
    CHECK_IF(nfsm == NULL, return NFSM_FAIL, "nfsm is null");
    CHECK_IF(ev == NULL, return NFSM_FAIL, "ev is null");
    CHECK_IF(ev->type < 0, return NFSM_FAIL, "ev->type = %d invalid", ev->type);

    if (nfsm->busy)
    {
        struct nfsmev* new_ev = calloc(sizeof(struct nfsmev), 1);
        *new_ev = *ev;
        if (FQUEUE_OK != fqueue_push(nfsm->evqueue, new_ev))
        {
            free(new_ev);
            return NFSM_FAIL;
        }
        return NFSM_OK;
    }

    nfsm->busy = true;

    _handle_ev(nfsm, ev);

    struct nfsmev* qev;
    FQUEUE_FOREACH(nfsm->evqueue, qev)
    {
        _handle_ev(nfsm, qev);
        free(qev);
    }

    nfsm->busy = false;
    return NFSM_OK;
}

struct nfsmst* nfsm_curr(struct nfsm* nfsm)
{
    CHECK_IF(nfsm == NULL, return NULL, "nfsm is null");
    return nfsm->curr;
}
