#include "basic.h"

#include "simple_sm.h"

enum
{
    EVENT1,
    EVENT2,
    EVENT3,

    MAX_EVENTS
};

static tSmStatus _action(tSm* sm, tSmEv* ev)
{
    dprint("here");
    return SM_OK;
}

static tSmStatus _action_new(tSm* sm, tSmEv* ev)
{
    dprint("here");
    sm_sendEv(sm, EVENT1, 0, 0);
    sm_sendEv(sm, EVENT1, 0, 0);
    return SM_OK;
}

static int _guard2(tSm* sm, tSmSt* st, tSmEv* ev)
{
    long val = (long)ev->arg1;
    return (int8_t)val;
}

static tSmStatus _enterSt(tSm* sm, tSmSt* st)
{
    dprint("enter %s", st->name);
    return SM_OK;
}

static tSmStatus _exitSt(tSm* sm, tSmSt* st)
{
    dprint("exit %s", st->name);
    return SM_OK;
}

int main(int argc, char const *argv[])
{
    tSm sm;

    sm_init(&sm, "tacolin", MAX_EVENTS);

    tSmSt* root = sm_rootSt(&sm);

    tSmSt* state1 = sm_createSt(&sm, "state1", _enterSt, _exitSt);
    sm_addSt(&sm, root, state1, 1);

    tSmSt* state2 = sm_createSt(&sm, "state2", _enterSt, _exitSt);
    sm_addSt(&sm, root, state2, 0);

    tSmSt* sub1 = sm_createSt(&sm, "sub_state1", _enterSt, _exitSt);
    sm_addSt(&sm, state1, sub1, 1);

    tSmSt* sub2 = sm_createSt(&sm, "sub_state2", _enterSt, _exitSt);
    sm_addSt(&sm, state1, sub2, 0);

    tSmTrans* trans = sm_createTrans(&sm, EVENT1, NULL, 0, "state2", _action);
    sm_addTrans(&sm, state1, trans);

    trans = sm_createTrans(&sm, EVENT1, NULL, 0, "state1", _action);
    sm_addTrans(&sm, state2, trans);

    trans = sm_createTrans(&sm, EVENT2, NULL, 0, "sub_state2", _action);
    sm_addTrans(&sm, sub1, trans);

    trans = sm_createTrans(&sm, EVENT3, NULL, 0, "sub_state1", _action_new);
    sm_addTrans(&sm, sub2, trans);

    sm_start(&sm);

    dprint("===");

    sm_sendEv(&sm, EVENT1, 0, 0);

    dprint("===");

    sm_sendEv(&sm, EVENT1, 0, 0);

    dprint("===");

    sm_sendEv(&sm, EVENT2, 0, 0);

    dprint("===");

    sm_sendEv(&sm, EVENT3, 0, 0);

    dprint("===");

    sm_stop(&sm);

    sm_uninit(&sm);

    return 0;
}
