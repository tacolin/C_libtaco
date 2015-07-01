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
    struct history* his = (struct history*)history_create(20, 10);

    history_addstr(his, "test 1");
    history_addstr(his, "test 2");
    history_addstr(his, "test 3");
    history_addstr(his, "test 4");
    history_addstr(his, "test 5");
    history_addstr(his, "test 6");
    history_addstr(his, "test 7");
    history_addstr(his, "test 8");
    history_addstr(his, "test 9");
    history_addstr(his, "test 10");
    history_addstr(his, "test 11");
    history_addstr(his, "test 12");
    history_addstr(his, "test 13");
    history_addstr(his, "test 14");
    history_addstr(his, "test 15");
    history_addstr(his, "test 166666666666666666666666666666666666666666666666666666");

    history_do_all(his, _print_str, NULL);
    dprint("===");
    history_do(his, 1, 3, _print_str, NULL);
    dprint("===");
    history_do(his, 9, 9, _print_str, NULL);
    dprint("===");
    history_do(his, 6, 8, _print_str, NULL);

    history_release(his);

    his = (struct history*)history_create(sizeof(struct taco), 10);

    int i;
    struct taco t;
    for (i=0; i<19; i++)
    {
        t.num = i*10;
        history_add(his, &t, sizeof(struct taco));
    }

    history_do_all(his, _print_taco, NULL);

    history_release(his);

    dprint("ok");
    return 0;
}
