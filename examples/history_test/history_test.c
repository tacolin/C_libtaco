#include "basic.h"
#include "history.h"

static void _print_str(int idx, void* data, void* arg)
{
    dprint("[%d] : %s", idx, (char*)data);
}

struct taco
{
    int num;
};

static void _print_taco(int idx, void* data, void* arg)
{
    struct taco* t = (struct taco*)data;
    dprint("[%d] : num = %d", idx, t->num);
}

int main(int argc, char const *argv[])
{
    struct history* h = history_create(20, 10);

    history_addstr(h, "test 1");
    history_addstr(h, "test 2");
    history_addstr(h, "test 3");
    history_addstr(h, "test 4");
    history_addstr(h, "test 5");
    history_addstr(h, "test 6");
    history_addstr(h, "test 7");
    history_addstr(h, "test 8");
    history_addstr(h, "test 9");
    history_addstr(h, "test 10");
    history_addstr(h, "test 11");
    history_addstr(h, "test 12");
    history_addstr(h, "test 13");
    history_addstr(h, "test 14");
    history_addstr(h, "test 15");
    history_addstr(h, "test 166666666666666666666666666666666666666666666666666666");

    history_do_all(h, _print_str, NULL);
    dprint("===");
    history_do(h, 1, 3, _print_str, NULL);
    dprint("===");
    history_do(h, 10, 10, _print_str, NULL);
    dprint("===");
    history_do(h, 6, 8, _print_str, NULL);

    history_release(h);

    h = history_create(sizeof(struct taco), 10);

    int i;
    struct taco t;
    for (i=0; i<19; i++)
    {
        t.num = i*10;
        history_add(h, &t, sizeof(struct taco));
    }

    history_do_all(h, _print_taco, NULL);

    history_release(h);

    dprint("ok");
    return 0;
}
