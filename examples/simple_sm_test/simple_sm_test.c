#include "basic.h"
#include "simplesm.h"

static tSmStatus _entrySt(tSmSt* state)
{
    check_if(state == NULL, return SM_ERROR, "state is null");
    dprint("enter %s", state->name);
    return SM_OK;
}

static tSmStatus _exitSt(tSmSt* state)
{
    check_if(state == NULL, return SM_ERROR, "state is null");
    dprint("exit %s", state->name);
    return SM_OK;
}

static tSmStatus _trans1to2(tSmSt* start, void* ev_data)
{
    check_if(start == NULL, return SM_ERROR, "start is null");
    return SM_OK;
}

static tSmStatus _trans2to1(tSmSt* start, void* ev_data)
{
    check_if(start == NULL, return SM_ERROR, "start is null");
    return SM_OK;
}

static int _guard2(tSmSt* start, void* ev_data)
{
    return 0;
}

static tSmStatus _trans2to3(tSmSt* start, void* ev_data)
{
    check_if(start == NULL, return SM_ERROR, "start is null");
    return SM_OK;
}

static tSmStatus _trans3to1(tSmSt* start, void* ev_data)
{
    check_if(start == NULL, return SM_ERROR, "start is null");
    return SM_OK;
}

////////////////////////////////////////////////////////////////////////////////

static tSmSt _root =
{
    .name = "outer_sm",
    .on_entry = _entrySt,
    .on_exit  = _exitSt,

    .sub_states = (tSmSt[])
    {
        {
            .name = "sub_state1",
            .on_entry = _entrySt,
            .on_exit = _exitSt,
            .is_init_state = 1,

            .transitions = (tSmTrans[])
            {
                {"event1", NULL, 0, "sub_state2", _trans1to2},
                {NULL}
            },
        },

        {
            .name = "sub_state2",
            .on_entry = _entrySt,
            .on_exit = _exitSt,

            .transitions = (tSmTrans[])
            {
                {"event2", _guard2, 0, "sub_state3", _trans2to3},
                {"event2", _guard2, 1, "sub_state1", _trans2to1},
                {NULL}
            },
        },

        {
            .name = "sub_state3",
            .on_entry = _entrySt,
            .on_exit = _exitSt,

            .transitions = (tSmTrans[])
            {
                {"event3", NULL, 0, "sub_state1", _trans3to1},
                {NULL}
            },
        },

        {NULL}
    },
};

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    sm_complete(&_root, NULL, NULL);
    dprint("hello");
    return 0;
}
