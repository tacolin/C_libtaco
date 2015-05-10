#include "basic.h"
#include "history.h"

static void _print_str(int idx, void* data)
{
    dprint("[%d] : %s", idx, (char*)data);
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

    history_do_all(h, _print_str);
    dprint("===");
    history_do(h, 1, 3, _print_str);
    dprint("===");
    history_do(h, 10, 10, _print_str);
    dprint("===");
    history_do(h, 6, 8, _print_str);


    history_release(h);
    dprint("ok");
    return 0;
}
