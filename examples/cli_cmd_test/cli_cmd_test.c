#include "basic.h"
#include "cli_cmd.h"

static int _help_fn(struct cli_cmd* cmd, struct cli* cli, int argc, char* argv[])
{
    dprint("your input is un-correct");
    return CMD_OK;
}

static int _show_me_the_money(struct cli_cmd* cmd, struct cli* cli, int argc, char* argv[])
{
    dprint("here");
    return CMD_OK;
}

int main(int argc, char const *argv[])
{
    cli_sys_init();
    cli_install_regular(_help_fn);

    int exec_id = 0;

    cli_install_node(exec_id, ">");

    struct cli_cmd* c;
    c = cli_install_cmd(NULL, "show", NULL, exec_id, "this is show");
    c = cli_install_cmd(c, "me", NULL, exec_id, "this is me");
    c = cli_install_cmd(c, "the", NULL, exec_id, "this is the");
    cli_install_cmd(c, "money", _show_me_the_money, exec_id, "this is money");

    cli_show_cmds(exec_id);

    cli_compare_string(exec_id, "show me the money");
    cli_compare_string(exec_id, "show me the fucker");

    cli_sys_uninit();
    dprint("ok");
    return 0;
}
