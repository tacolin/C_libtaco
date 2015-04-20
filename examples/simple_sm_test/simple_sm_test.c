#include "basic.h"

#include "simple_sm.h"

static tSmStatus _action(tSm* sm, void* ev_data)
{
    dprint("here");
    return SM_OK;
}

static int _guard2(tSm* sm, tSmSt* st, void* ev_data)
{
    long val = (long)ev_data;
    return (int)val;
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

    sm_init(&sm, "tacolin");

    tSmSt* root = sm_rootSt(&sm);

    tSmSt* state1 = sm_createSt(&sm, "state1", _enterSt, _exitSt);
    sm_addSt(&sm, root, state1, 1);

    tSmSt* state2 = sm_createSt(&sm, "state2", _enterSt, _exitSt);
    sm_addSt(&sm, root, state2, 0);

    tSmTrans* trans = sm_createTrans(&sm, "event1", NULL, 0, "state2", _action);
    sm_addTrans(&sm, state1, trans);

    trans = sm_createTrans(&sm, "event1", NULL, 0, "state1", _action);
    sm_addTrans(&sm, state2, trans);

    tSmSt* sub1 = sm_createSt(&sm, "sub_state1", _enterSt, _exitSt);
    sm_addSt(&sm, state1, sub1, 1);

    tSmSt* sub2 = sm_createSt(&sm, "sub_state2", _enterSt, _exitSt);
    sm_addSt(&sm, state1, sub2, 0);

    trans = sm_createTrans(&sm, "event2", NULL, 0, "sub_state2", _action);
    sm_addTrans(&sm, sub1, trans);

    trans = sm_createTrans(&sm, "event3", NULL, 0, "sub_state1", _action);
    sm_addTrans(&sm, sub2, trans);

    sm_start(&sm);

    dprint("");

    sm_sendEv(&sm, "event1", NULL);

    dprint("");

    sm_sendEv(&sm, "event1", NULL);

    dprint("");

    sm_stop(&sm);

    sm_uninit(&sm);

    return 0;
}
