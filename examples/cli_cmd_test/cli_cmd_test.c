#include "array.h"
#include "basic.h"
#include "cli_cmd.h"

static int _help_fn(struct cli* cli, int argc, char* argv[])
{
    dprint("your input is un-correct : %s", argv[0]);
    return CMD_OK;
}

DEFUN(_show_me_the_money,
      _show_me_the_money_cmd,
      "show me the money",
      "desc 1",
      "desc 2",
      "desc 3",
      "desc 4")
{
    dprint("here");
    return CMD_OK;
}

DEFUN(_set_id,
      _set_id_cmd,
      "set id <1-100>",
      "desc 1",
      "desc 2")
{
    dprint("id = %d", atoi(argv[0]));
    return CMD_OK;
}

int main(int argc, char const *argv[])
{
    cli_sys_init();
    cli_install_regular(_help_fn);

    int exec_id = 0;

    cli_install_node(exec_id, ">");

    cli_install_cmd(exec_id, &_show_me_the_money_cmd);
    cli_install_cmd(exec_id, &_set_id_cmd);

    cli_show_cmds(exec_id);

    dprint("show me the money:");
    cli_excute_cmd(exec_id, "show me the money");

    dprint("sh m t mon");
    cli_excute_cmd(exec_id, "sh m t mon");

    dprint("show me the fucker");
    cli_excute_cmd(exec_id, "show me the fucker");

    dprint("set id 50");
    cli_excute_cmd(exec_id, "set id 50");

    dprint("set id 101");
    cli_excute_cmd(exec_id, "set id 101");

    dprint("se i 1");
    cli_excute_cmd(exec_id, "se i 1");

    char* output;
    struct array* completions = cli_get_completions(exec_id, "se i 1 ", &output);
    dprint("output = %s", output);
    int i;
    for(i=0; i<completions->num; i++)
    {
        dprint("completions[%d] = %s", i, (char*)completions->datas[i]);
    }
    free(output);
    array_release(completions);

    cli_sys_uninit();
    dprint("ok");
    return 0;
}
