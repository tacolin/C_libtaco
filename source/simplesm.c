#include "simplesm.h"

static void _defaultQueueCleanFn(void* content)
{
    tSmEv* ev = (tSmEv*)content;

    check_if(content == NULL, return, "content is null");

    if (ev->fn && ev->ev_data)
    {
        ev->fn(ev->ev_data);
        ev->ev_data = NULL;
    }
}

tSmStatus sm_complete(tSmSt* state, void* db, tSmEvDataFreeFn fn)
{
    check_if(state == NULL, return SM_ERROR, "state is null");

    int i;

    state->db = db;
    state->is_init_state = 1;

    if (state->sub_states)
    {
        for (i=0; state->sub_states[i].name; i++);
        state->sub_state_num = i;
    }

    if (state->transitions)
    {
        for (i=0; state->transitions[i].ev_name; i++);
        state->transition_num = i;
    }

    queue_init(&state->evqueue, state->name, QUEUE_EV_NUM, _defaultQueueCleanFn,
               QUEUE_UNSUSPEND, QUEUE_UNSUSPEND);

    for (i=0; i<state->sub_state_num; i++)
    {
        sm_complete(&state->sub_states[i], db, fn);
    }

    state->is_started = 0;
    state->is_busy = 0;
    state->curr_sub_state = NULL;
    state->is_completed = 1;

    return SM_OK;
}

tSmStatus sm_start(tSmSt* root);
tSmStatus sm_stop(tSmSt* root);

tSmStatus sm_sendev(tSmSt* root, char* ev_name, void* ev_data);
