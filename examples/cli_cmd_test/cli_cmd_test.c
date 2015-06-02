#include "basic.h"
#include "cli_cmd.h"

DEFUN(show_me_the_money,
      show_me_the_money_cmd,
      "show me the money",
      "this is show",
      "this is me",
      "this is the",
      "this is money")
{
    dprint("lalala - show me the money, i am pool");
    return CMD_OK;
}

DEFUN(test_alternative,
      test_alternative_cmd,
      "(add | del | modify) station <1-100>",
      "this is add, del, or modify",
      "this is station",
      "this is station id")
{
    dprint("fff - this is alternative cmd test");
    return CMD_OK;
}

int main(int argc, char const *argv[])
{
    cli_sys_init();

    cli_install_node("exec", ">");
    cli_install_node("enable", "#");
    cli_install_node("config", "(config)#");
    cli_install_node("interface", "(config-if)#");
    cli_set_default_node("exec");

    cli_install_cmd("exec", &show_me_the_money_cmd);
    cli_install_cmd("exec", &test_alternative_cmd);

    cli_show_cmds("exec");

    cli_sys_uninit();
    dprint("ok");
    return 0;
}
