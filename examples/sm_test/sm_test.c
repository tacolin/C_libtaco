#include "basic.h"
#include <string.h>

struct tState;
struct tEvent;

typedef int (*tStateEntry)(struct tState* state);
typedef int (*tStateExit)(struct tState* state);

typedef int (*tTransGuard)(struct tState* state, struct tEvent event);
typedef int (*tTransAction)(struct tState* state, struct tEvent event);

typedef struct tEvent 
{
    int event_value;
    void* arg;

} tEvent;

typedef struct tState 
{
    char* name;

    struct tState* sub_states;

    tStateEntry on_entry;
    tStateExit on_exit;

    struct tTransition
    {
        char* begin;
        struct tState* begin_state;
        int event_value;
        
        tTransGuard on_guard;
        int guard_value;

        tTransAction on_action;
        char* end;
        struct tState* end_state;

    }* transitions;

    int is_init_state;
    int is_root;

} tState;

#define _state2 \
{\
    .name = "state2", \
    .is_init_state = 0,\
}

static tState _simple = 
{
    .name = "simple1",
    .is_root = 1,    

    .sub_states = (tState[])
    {
        {
            .name = "state1",
            .is_init_state = 1,
        },

        _state2
    },

    .transitions = (struct tTransition[])
    {
        {
            .begin = "state1",
            .end = "state2",
            .event_value = 1,
        },
    },
};

int main(int argc, char const *argv[])
{
    dprint("hello");    
    return 0;
}