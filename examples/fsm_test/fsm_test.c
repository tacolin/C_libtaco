#include <stdint.h>

#include "basic.h"
#include "fsm.h"

//   o
//   |
//   v
// +---+                    +-----+
// | 1 |                    |     |
// +---+                    v     |(c)
//   |(a)                +---+    |
//   |          +------->| 3 |----+
//   v          | (true) +---+
// +---+  (b)   |
// | 2 |--------+
// +---+        |
//   ^          | (false)+---+
//   |          +------->| 4 |
//   |                   +---+
//   |                     |
//   |    (d)   +---+      |(d)
//   +----------| 5 | <----+
//              +---+

struct taco
{
    int data;
};

static void _action(struct fsm* fsm, void* db, struct fsmev* ev)
{
    struct taco* t = (struct taco*)(db);
    dprint("t->data = %d", t->data);
    dprint("ev->type = %c, ev->data = %ld", (char)(ev->type), (long)(intptr_t)(ev->data));
}

static void _entry_st(struct fsm* fsm, void* db, struct fsmst* st)
{
    dprint("enter %s", st->name);
    struct taco* t = (struct taco*)(db);
    dprint("t->data = %d", t->data);
    dprint("");
}

static void _exit_st(struct fsm* fsm, void* db, struct fsmst* st)
{
    dprint("exit %s", st->name);
    struct taco* t = (struct taco*)(db);
    dprint("t->data = %d", t->data);
}

static int _guard(struct fsm* fsm, void* db, struct fsmev* ev)
{
    struct taco* t = (struct taco*)(db);
    dprint("t->data = %d, ev->data = %ld", t->data, (long)(intptr_t)(ev->data));
    if (t->data > (intptr_t)(ev->data))
    {
        dprint("return true");
        return true;
    }
    else
    {
        dprint("return false");
        return false;
    }
}

static void _goto_s2(struct fsm* fsm, void* db, struct fsmst* st)
{
    dprint("enter %s", st->name);
    struct taco* t = (struct taco*)(db);
    dprint("t->data = %d", t->data);

    struct fsmev new_ev = {.type = (int)'d'};
    fsm_sendev(fsm, &new_ev);
}

static struct fsmst _states[];
static struct fsmst _states[] = {
    {
        .name = "s1",
        .entryfn = _entry_st,
        .exitfn  = _exit_st,
        .trans_array = (struct fsmtrans [])
        {
            {(int)'a', _action, 0, 0, &_states[1]},
            {-1} // end
        },
    },

    {
        .name = "s2",
        .entryfn = _entry_st,
        .exitfn  = _exit_st,
        .trans_array = (struct fsmtrans [])
        {
            {(int)'b', _action, _guard, (int)true, &_states[2]},
            {(int)'b', _action, _guard, (int)false, &_states[3]},
            {-1} // end
        },
    },

    {
        .name = "s3",
        .entryfn = _entry_st,
        .exitfn  = _exit_st,
        .trans_array = (struct fsmtrans [])
        {
            {(int)'c', _action, 0, 0,  &_states[2]},
            {-1} // end
        },
    },

    {
        .name = "s4",
        .entryfn = _entry_st,
        .exitfn  = _exit_st,
        .trans_array = (struct fsmtrans [])
        {
            {(int)'d', 0, 0, 0,  &_states[4]},
            {-1} // end
        },
    },

    {
        .name = "s5",
        .entryfn = _goto_s2,
        .trans_array = (struct fsmtrans [])
        {
            {(int)'d', _action, 0, 0,  &_states[1]},
            {-1} // end
        },
    },
};

int main(int argc, char const *argv[])
{
    struct taco taco_db = {.data = 100};
    struct fsm fsm;
    fsm_init(&fsm, &_states[0], &taco_db);
    dprint("curr = %s", fsm_curr(&fsm)->name);

    struct fsmev ev;

    //////////////////////////
    // s1 -> s2 -> s3 -> s3
    //////////////////////////
    // ev.type = (int)'a';
    // ev.data = (void*)((intptr_t)0);
    // fsm_sendev(&fsm, &ev);
    // dprint("curr = %s", fsm_curr(&fsm)->name);

    // ev.type = (int)'b';
    // ev.data = (void*)((intptr_t)50);
    // fsm_sendev(&fsm, &ev);
    // dprint("curr = %s", fsm_curr(&fsm)->name);

    // ev.type = (int)'b';
    // ev.data = (void*)((intptr_t)0);
    // fsm_sendev(&fsm, &ev);
    // dprint("curr = %s", fsm_curr(&fsm)->name);

    // ev.type = (int)'c';
    // ev.data = (void*)((intptr_t)0);
    // fsm_sendev(&fsm, &ev);
    // dprint("curr = %s", fsm_curr(&fsm)->name);

    ///////////////////////////////
    // s1 -> s2 -> s4 -> s5 -> s2
    ///////////////////////////////
    ev.type = (int)'a';
    ev.data = (void*)((intptr_t)0);
    fsm_sendev(&fsm, &ev);
    dprint("curr = %s", fsm_curr(&fsm)->name);

    ev.type = (int)'b';
    ev.data = (void*)((intptr_t)150);
    fsm_sendev(&fsm, &ev);
    dprint("curr = %s", fsm_curr(&fsm)->name);

    ev.type = (int)'d';
    ev.data = (void*)((intptr_t)0);
    fsm_sendev(&fsm, &ev);
    dprint("curr = %s", fsm_curr(&fsm)->name);

    fsm_uninit(&fsm);
    dprint("ok");
    return 0;
}
