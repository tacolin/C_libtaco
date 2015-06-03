#include "array.h"
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

static int _set_id(struct cli_cmd* cmd, struct cli* cli, int argc, char* argv[])
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

    c = cli_install_cmd(NULL, "set", NULL, exec_id, "this is set");
    c = cli_install_cmd(c, "id", NULL, exec_id, "this is id");
    cli_install_cmd(c, "<1-100>", _set_id, exec_id, "this is value");

    cli_show_cmds(exec_id);

    cli_excute_cmd(exec_id, "show me the money");
    cli_excute_cmd(exec_id, "sh m t mon");
    cli_excute_cmd(exec_id, "show me the fucker");
    cli_excute_cmd(exec_id, "set id 50");
    cli_excute_cmd(exec_id, "set id 101");

    char* output;
    struct array* completions = cli_get_completions(exec_id, "s ", &output);
    dprint("output = %s", output);
    int i;
    for(i=0; i<completions->num; i++)
    {
        dprint("completions[%d] = %s", i, (char*)completions->datas[i]);
    }
    free(output);
    cli_release_array(completions);

    cli_sys_uninit();
    dprint("ok");
    return 0;
}
