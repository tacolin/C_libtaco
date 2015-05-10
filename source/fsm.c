#include <stdio.h>
#include <stdlib.h>
#include "fsm.h"

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

static void _clean_fsmev(void* input)
{
    if (input) free(input);
}

int fsm_init(struct fsm* fsm, struct fsmst* init_st, void* db)
{
    CHECK_IF(fsm == NULL, return FSM_FAIL, "fsm is null");
    CHECK_IF(init_st == NULL, return FSM_FAIL, "init_st is null");

    fsm->curr    = init_st;
    fsm->prev    = NULL;
    fsm->busy    = false;
    fsm->db      = db;
    fsm->evqueue = fqueue_create(_clean_fsmev);
    CHECK_IF(fsm->evqueue == NULL, return FSM_FAIL, "fqueue_create failed");

    if (init_st->entryfn) init_st->entryfn(fsm, db, init_st);

    return FSM_OK;
}

void fsm_uninit(struct fsm* fsm)
{
    CHECK_IF(fsm == NULL, return, "fsm is null");

    fqueue_release(fsm->evqueue);
    fsm->curr    = NULL;
    fsm->prev    = NULL;
    fsm->evqueue = NULL;
    fsm->busy    = false;
    return;
}

static struct fsmtrans* _find_trans(struct fsm* fsm, struct fsmst* st, struct fsmev* ev)
{
    CHECK_IF(fsm == NULL, return NULL, "fsm is null");
    CHECK_IF(st == NULL, return NULL, "st is null");
    CHECK_IF(ev == NULL, return NULL, "ev is null");

    int i;
    for (i=0; st->trans_array[i].ev_type != INVALID_EV_TYPE; i++)
    {
        struct fsmtrans* trans = &(st->trans_array[i]);
        if (trans->ev_type == ev->type)
        {
            if (trans->guard == NULL)
            {
                return trans;
            }
            else if (trans->guard(fsm, fsm->db, ev) == trans->guard_val)
            {
                return trans;
            }
        }
    }
    return NULL;
}

static void _handle_ev(struct fsm* fsm, struct fsmev* ev)
{
    struct fsmst* curr     = fsm->curr;
    struct fsmtrans* trans = _find_trans(fsm, curr, ev);
    if (trans == NULL) return;

    struct fsmst* next = trans->next_st;
    if (next)
    {
        // even if next = curr, ev will trigger state transition
        // exit curr state --> do transition --> enter next state (or enter curr state again)
        if (curr->exitfn)  curr->exitfn(fsm, fsm->db, curr);

        if (trans->action) trans->action(fsm, fsm->db, ev);

        if (next->entryfn) next->entryfn(fsm, fsm->db, next);
    }
    else
    {
        // no next state : handle ev in curr state, but not trigger state transition
        next = curr;
        if (trans->action) trans->action(fsm, fsm->db, ev);
    }
    fsm->prev = curr;
    fsm->curr = next;
    return;
}

int fsm_sendev(struct fsm* fsm, struct fsmev* ev)
{
    CHECK_IF(fsm == NULL, return FSM_FAIL, "fsm is null");
    CHECK_IF(ev == NULL, return FSM_FAIL, "ev is null");
    CHECK_IF(ev->type < 0, return FSM_FAIL, "ev->type = %d invalid", ev->type);

    if (fsm->busy)
    {
        struct fsmev* new_ev = calloc(sizeof(struct fsmev), 1);
        *new_ev = *ev;
        if (FQUEUE_OK != fqueue_push(fsm->evqueue, new_ev))
        {
            free(new_ev);
            return FSM_FAIL;
        }
        return FSM_OK;
    }

    fsm->busy = true;

    _handle_ev(fsm, ev);

    struct fsmev* qev;
    FQUEUE_FOREACH(fsm->evqueue, qev)
    {
        _handle_ev(fsm, qev);
        free(qev);
    }

    fsm->busy = false;
    return FSM_OK;
}

struct fsmst* fsm_curr(struct fsm* fsm)
{
    CHECK_IF(fsm == NULL, return NULL, "fsm is null");
    return fsm->curr;
}
