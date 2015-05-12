#include <stdint.h>
#include "nested_fsm.h"
#include "basic.h"

//                        (10)
//     o            +-------------+           +---+
//     |            |             |           |   |
//     v            v             |           v   |
// +----------------------------------------------|----+
// | group                      (true)  +---+ (3) |    |
// |                           +------->| a |-----+    |
// |    +------+ (1) +---+ (2) |        +---+          |
// | o->| idle |---->| h |-----+                       |
// |    +------+     +---+     |        +---+ (3)      |
// |       ^           ^       +------->| i |-----+    |
// |       |           |        (false) +---+     |    |
// +-------|-----------|--------------------------|----+
//         |           |         |                |
//         |           |   (11)  |                |
//         |           +---------+                |
//         +--------------------------------------+

struct taco
{
    int data;
};

static void _action(struct nfsm* nfsm, void* db, struct nfsmev* ev)
{
    struct taco* t = (struct taco*)(db);
    dprint("t->data = %d", t->data);
    dprint("ev->type = %d, ev->data = %ld", ev->type, (long)(intptr_t)(ev->data));
}

static int _guard(struct nfsm* nfsm, void* db, struct nfsmev* ev)
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

struct nfsmst _group, _idle, _h, _a, _i;

struct nfsmst _group =
{
    .name = "group",
    .init_subst = &_idle,
    .trans_array = (struct nfsmtrans [])
    {
        {10, _action, 0, 0, &_group},
        {11, _action, 0, 0, &_h},
        {-1}
    },
};

struct nfsmst _idle =
{
    .name = "idle",
    .parent = &_group,
    .trans_array = (struct nfsmtrans [])
    {
        {1, _action, 0, 0, &_h},
        {-1}
    },
};

struct nfsmst _h =
{
    .name = "h",
    .parent = &_group,
    .trans_array = (struct nfsmtrans [])
    {
        {2, _action, _guard, (int)true, &_a},
        {2, _action, _guard, (int)false, &_i},
        {-1}
    },
};

struct nfsmst _a =
{
    .name = "a",
    .parent = &_group,
    .trans_array = (struct nfsmtrans [])
    {
        {3, _action, 0, 0, &_group},
        {-1}
    },
};

struct nfsmst _i =
{
    .name = "i",
    .parent = &_group,
    .trans_array = (struct nfsmtrans [])
    {
        {3, _action, 0, 0, &_idle},
        {-1}
    },
};

int main(int argc, char const *argv[])
{
    struct taco taco_db = {.data = 100};
    struct nfsm nfsm;

    nfsm_init(&nfsm, &_group, &taco_db);
    dprint("curr = %s", nfsm_curr(&nfsm)->name);

    struct nfsmev ev = {};

    ////////////////////////////////////////////////
    // idle -> h -> a -> group(-> idle)
    ////////////////////////////////////////////////
    // ev.type = 1;
    // ev.data = NULL;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    // ev.type = 2;
    // ev.data = (void*)(intptr_t)50;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    // ev.type = 3;
    // ev.data = NULL;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    ////////////////////////////////////////////////
    // idle -> h -> i -> idle
    ////////////////////////////////////////////////
    // ev.type = 1;
    // ev.data = NULL;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    // ev.type = 2;
    // ev.data = (void*)(intptr_t)150;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    // ev.type = 3;
    // ev.data = NULL;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    ////////////////////////////////////////////////
    // idle -> h -> i -> group(idle)
    ////////////////////////////////////////////////
    // ev.type = 1;
    // ev.data = NULL;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    // ev.type = 2;
    // ev.data = (void*)(intptr_t)150;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    // ev.type = 10;
    // ev.data = NULL;
    // nfsm_sendev(&nfsm, &ev);
    // dprint("curr = %s", nfsm_curr(&nfsm)->name);

    ////////////////////////////////////////////////
    // idle -> h -> a -> h -> group(->idle)
    ////////////////////////////////////////////////
    ev.type = 1;
    ev.data = NULL;
    nfsm_sendev(&nfsm, &ev);
    dprint("curr = %s", nfsm_curr(&nfsm)->name);

    ev.type = 2;
    ev.data = (void*)(intptr_t)50;
    nfsm_sendev(&nfsm, &ev);
    dprint("curr = %s", nfsm_curr(&nfsm)->name);

    ev.type = 11;
    ev.data = NULL;
    nfsm_sendev(&nfsm, &ev);
    dprint("curr = %s", nfsm_curr(&nfsm)->name);

    ev.type = 10;
    ev.data = NULL;
    nfsm_sendev(&nfsm, &ev);
    dprint("curr = %s", nfsm_curr(&nfsm)->name);

    nfsm_uninit(&nfsm);
    dprint("ok");
    return 0;
}
