// #include "basic.h"
// #include "tcp.h"

// #include <signal.h>
// #include <string.h>

// #define CTRL(c) (c - '@')

// #define MAX_CMD_SIZE 100

// static int _running = 1;

// static void _sigIntHandler(int sig_num)
// {
//     _running = 0;
//     dprint("stop main()");
// }

// static void _clearLine(struct tcp* tcp, char *cmd, int len)
// {
//     int i;

//     // 先把游標退到頭
//     for (i=0; i<len; i++)
//     {
//         tcp_send(tcp, "\b", 1);
//     }

//     // 把游標從頭一直寫空白寫到尾
//     for (i=0; i<len; i++)
//     {
//         tcp_send(tcp, " ", 1);
//     }

//     // 再把游標退到頭
//     for (i=0; i<len; i++)
//     {
//         tcp_send(tcp, "\b", 1);
//     }

//     // 把 cmd 全部 memset 掉
//     memset(cmd, 0, len);
// }

// static int _process(struct tcp* tcp)
// {
//     static unsigned char c;
//     static char cmd[MAX_CMD_SIZE] = {0};
//     static int len = 0;
//     static char* oldcmd = NULL;
//     static int oldlen = 0;
//     static int cursor = 0;
//     static char prompt[] = "[taco]$ ";
//     static int is_option = 0;
//     static int esc = 0;
//     static int pre = 1;
//     static char lastchar = 0;
//     static int in_history = 0;
//     static int insert_mode = 1;
//     static int is_delete = 0;

//     if (pre)
//     {
//         // 把 oldcmd 變成現在的 cmd
//         if (oldcmd)
//         {
//             len = oldlen;
//             cursor = oldlen;
//             pre = 1;
//             oldcmd = NULL;
//             oldlen = 0;
//         }
//         else
//         {
//             memset(cmd, 0, MAX_CMD_SIZE);
//             len = 0;
//             cursor = 0;
//         }

//         // 印出 prompt
//         tcp_send(tcp, prompt, strlen(prompt));
//         pre = 0;
//     }

//     // 接收資料
//     int recvlen = tcp_recv(tcp, &c, 1);
//     if (recvlen < 0)
//     {
//         derror("tcp_recv failed");
//         goto _ERROR;
//     }
//     else if (recvlen == 0)
//     {
//         len = -1;
//         goto _BREAK;
//     }

//     dprint("%x %d %c, len = %d", c, c, c, len);

//     // 如果收到 255，表示後面一個byte是用來做option的，先標記起來，然後收下一個
//     if (c == 255 && is_option == 0)
//     {
//         dprint("option");
//         is_option++;
//         goto _CONT;
//     }

//     // 如果前一個收到的是255，就會標記option
//     if (is_option)
//     {
//         // 如果在 251~254 之間，就是option的值，繼續收
//         if (c >= 251 && c <= 254)
//         {
//             is_option = c;
//             goto _CONT;
//         }

//         // 如果不是 251~254，也不是 255，那就表示已經不是 option 了，設成0
//         if (c != 255)
//         {
//             is_option = 0;
//             goto _CONT;
//         }

//         // 如果收到 255 第二次，準備要處理之前收到的資料 (還是啥？這裡我搞不清楚為何不跳出去)
//         is_option = 0;
//     }

//     // 如果收到 27 表示是 ESC 控制字元，標記起來，等待下一個 byte
//     if (c == 27)
//     {
//         esc = 1;
//         goto _CONT;
//     }

//     // 如果上一個收到的是 esc 控制字元的話
//     if (esc)
//     {
//         // 處理上下左右箭頭鍵
//         if (esc == '[')
//         {
//             esc = 0;

//             switch (c)
//             {
//                 case 'A':
//                     c = CTRL('P');
//                     break;

//                 case 'B':
//                     c = CTRL('N');
//                     break;

//                 case 'C':
//                     c = CTRL('F');
//                     break;

//                 case 'D':
//                     c = CTRL('B');
//                     break;

//                 case '3':
//                     dprint("is_delete");
//                     is_delete = 1;
//                     goto _CONT;
//                     // break;

//                 default:
//                     c = 0;
//                     break;
//             }
//         }
//         else // 如果不是 [ 的控制字元我不想理會  (原因不明)
//         {
//             esc = (c == '[') ? c : 0;
//             goto _CONT;
//         }
//     }

//     if (is_delete)
//     {
//         if (c == 126 && cursor != len)
//         {
//             // 東湊西湊湊出 delete ...
//             int i;
//             for (i=cursor; i<= len; i++)
//             {
//                 cmd[i] = cmd[i+1];
//             }
//             tcp_send(tcp, cmd + cursor, strlen(cmd + cursor));
//             tcp_send(tcp, " ", 1);
//             for (i=0; i<= (int)strlen(cmd + cursor); i++)
//             {
//                 tcp_send(tcp, "\b", 1);
//             }

//             len--;
//         }

//         is_delete = 0;

//         goto _CONT;
//     }

//     // 收到空字元
//     if (c == 0) goto _CONT;

//     // 收到換行鍵
//     if (c == '\n') goto _CONT;

//     // 收到 return，換行之外還要記得要印 prompt
//     if (c == '\r')
//     {
//         tcp_send(tcp, "\r\n", 2);
//         goto _BREAK;
//     }

//     // 收到 ctrl c，那就回送 \a (alert)，telnet client 那邊應該會叮一聲響起來 ♪
//     if (c == CTRL('C'))
//     {
//         tcp_send(tcp, "\a", 1);
//         goto _CONT;
//     }

//     //  backword          backspace         delete
//     if (c == CTRL('W') || c == CTRL('H') || c == 0x7f)
//     {
//         int back = 0;

//         if (c == CTRL('W'))
//         {
//             if (len == 0) goto _CONT;
//             if (cursor == 0) goto _CONT;

//             int nc = cursor;

//             while (nc && cmd[nc-1] == ' ') // calculate back redundant space
//             {
//                 nc--;
//                 back++;
//             }

//             while (nc && cmd[nc-1] != ' ') // calculate back word
//             {
//                 nc--;
//                 back++;
//             }
//         }
//         else
//         {
//             // 如果 backspace 或 delete 已經退到頭了，那就會響警示音
//             if (len == 0 || cursor == 0)
//             {
//                 tcp_send(tcp, "\a", 1);
//                 goto _CONT;
//             }

//             back = 1;
//         }

//         if (back)
//         {
//             while (back--)
//             {
//                 if (len == cursor)
//                 {
//                     cmd[--cursor] = 0;
//                     tcp_send(tcp, "\b \b", 3);
//                 }
//                 else
//                 {
//                     int i;
//                     cursor--;
//                     for (i=cursor; i<= len; i++)
//                     {
//                         cmd[i] = cmd[i+1];
//                     }
//                     tcp_send(tcp, "\b", 1);
//                     tcp_send(tcp, cmd + cursor, strlen(cmd + cursor));
//                     tcp_send(tcp, " ", 1);
//                     for (i=0; i<= (int)strlen(cmd + cursor); i++)
//                     {
//                         tcp_send(tcp, "\b", 1);
//                     }
//                 }
//                 len--;
//             }

//             goto _CONT;
//         }
//     }

//     // CTRL L - Redraw ?
//     if (c == CTRL('L'))
//     {
//         int i;
//         int cursor_back = len - cursor;

//         tcp_send(tcp, "\r\n", 2);
//         tcp_send(tcp, prompt, strlen(prompt));

//         if (len > 0)
//         {
//             tcp_send(tcp, cmd, len);
//         }

//         for (i=0; i<cursor_back; i++)
//         {
//             tcp_send(tcp, "\b", 1);
//         }

//         goto _CONT;
//     }

//     // Clear Line
//     if (c == CTRL('U'))
//     {
//         _clearLine(tcp, cmd, len);
//         len = 0;
//         cursor = 0;
//         goto _CONT;
//     }

//     // Kill to the EOL
//     if (c == CTRL('K'))
//     {
//         if (cursor == len) goto _CONT;

//         int c;
//         for (c=cursor; c<len; c++)
//         {
//             tcp_send(tcp, " ", 1); // 把游標後面的東西都刪掉
//         }

//         for (c=cursor; c<len; c++)
//         {
//             tcp_send(tcp, "\b", 1); // 再把游標退回到原本的位置
//         }

//         memset(cmd + cursor, 0, len - cursor); // 把 cmd 相對應位置的部分清空
//         len = cursor;
//         goto _CONT;
//     }

//     // EOT
//     if (c == CTRL('D'))
//     {
//         dprint("here1, len = %d", len);
//         if (len > 0)
//         {
//             dprint("here2");
//             goto _CONT;
//         }

//         len = -1;
//         goto _ERROR;
//     }

//     // Disable
//     if (c == CTRL('Z'))
//     {
//         goto _BREAK;
//     }

//     // TAB
//     if (c == CTRL('I'))
//     {
//         if (cursor != len) goto _CONT;

//         // TODO: auto complete
//         //int num_completions = _getCompletions(...)
//         int num_completions = 0;

//         if (num_completions == 0)
//         {
//             tcp_send(tcp, "\a", 1);
//         }
//         else if (num_completions == 1)
//         {
//             // TODO: show completion
//         }
//         else if (lastchar == CTRL('I'))
//         {
//             // double TAB
//             tcp_send(tcp, "\r\n", 2);

//             // TODO: show all completions

//             tcp_send(tcp, "\r\n", 2);
//             goto _BREAK;
//         }
//         else
//         {
//             // more than 1 completion
//             lastchar = c;
//             tcp_send(tcp, "\a", 1);
//         }

//         goto _CONT;
//     }

//     // up or down, show history
//     if (c == CTRL('P') || c == CTRL('N'))
//     {
//         // TODO: show history
//         goto _CONT;
//     }

//     // left , move cursor
//     if (c == CTRL('B'))
//     {
//         if (cursor > 0)
//         {
//             tcp_send(tcp, "\b", 1);
//             cursor--;
//         }

//         goto _CONT;
//     }

//     // Right, move cursor
//     if (c == CTRL('F'))
//     {
//         if (cursor < len)
//         {
//             tcp_send(tcp, &cmd[cursor], 1);
//             cursor++;
//         }

//         goto _CONT;
//     }

//     // start of line
//     if (c == CTRL('A'))
//     {
//         if (cursor > 0)
//         {
//             tcp_send(tcp, "\r", 1);
//             tcp_send(tcp, prompt, strlen(prompt));
//             cursor = 0;
//         }

//         goto _CONT;
//     }

//     // end of line
//     if (c == CTRL('E'))
//     {
//         if (cursor < len)
//         {
//             tcp_send(tcp, &cmd[cursor], len - cursor);
//             cursor = len;
//         }

//         goto _CONT;
//     }

//     // normal character typed
//     if (cursor == len)
//     {
//         cmd[cursor] = c;
//         if (len < (MAX_CMD_SIZE-1))
//         {
//             len++;
//             cursor++;
//         }
//         else
//         {
//             tcp_send(tcp, "\a", 1);
//             goto _CONT;
//         }
//     }
//     else
//     {
//         // Middle of text
//         if (insert_mode)
//         {
//             // insert
//             if (len >= MAX_CMD_SIZE - 2) len--;

//             int i;
//             for (i=len; i>=cursor; i--)
//             {
//                 cmd[i+1] = cmd[i];
//             }

//             cmd[cursor] = c;

//             tcp_send(tcp, &cmd[cursor], len - cursor + 1);

//             for (i=0; i<(len - cursor + 1); i++)
//             {
//                 tcp_send(tcp, "\b", 1);
//             }

//             len++;
//         }
//         else
//         {
//             // replace
//             cmd[cursor] = c;
//         }
//         cursor++;
//     }

//     if (c == '?' && cursor == len)
//     {
//         tcp_send(tcp, "\r\n", 2);
//         oldcmd = cmd;
//         oldlen = len - 1;
//         cursor = len - 1;
//         goto _BREAK;
//     }

//     tcp_send(tcp, &c, 1);

//     oldcmd = 0;
//     oldlen = 0;
//     lastchar = c;

// _CONT:
//     return 0;

// _BREAK:
//     pre = 1;
//     return 0;

// _ERROR:
//     return -1;
// }

// int main(int argc, char const *argv[])
// {
//     signal(SIGINT, _sigIntHandler);

//     struct tcp_server server = {};
//     tcp_server_init(&server, NULL, 5000, 10);

//     struct tcp tcp = {};

//     tcp_server_accept(&server, &tcp);

//     const char negotiate[] =
//         "\xFF\xFB\x03"
//         "\xFF\xFB\x01"
//         "\xFF\xFD\x03"
//         "\xFF\xFD\x01";

//     tcp_send(&tcp, (void*)negotiate, strlen(negotiate));

//     int ret;

//     while (_running)
//     {
//         ret = _process(&tcp);
//         CHECK_IF(ret < 0, goto _ERROR, "_process failed");
//     }

//     dprint("over");
//     tcp_server_uninit(&server);
//     return 0;

// _ERROR:
//     dprint("error-over");
//     tcp_server_uninit(&server);
//     return -1;
// }

#include <stdio.h>
#include <signal.h>
#include <sys/select.h>

#include "basic.h"
#include "cli_server.h"

static int _running = 1;

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    dprint("stop main()");
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    struct cli_server server = {};
    struct cli cli = {};

    cli_server_init(&server, NULL, 5000, 10, "5566", "i am sad");
    // cli_server_init(&server, NULL, 5000, 10, NULL, NULL);

    fd_set readset;
    int select_ret;
    struct timeval timeout;

    while (_running)
    {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(server.fd, &readset);

        if (cli.is_init)
        {
            FD_SET(cli.fd, &readset);
        }

        select_ret = select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
        if (select_ret < 0)
        {
            derror("select_ret = %d", select_ret);
            break;
        }
        else if (select_ret == 0)
        {
            continue;
        }
        else
        {
            if (FD_ISSET(server.fd, &readset))
            {
                cli_server_accept(&server, &cli);
            }

            if (cli.is_init)
            {
                if (FD_ISSET(cli.fd, &readset))
                {
                    if (CLI_OK != cli_process(&cli))
                    {
                        break;
                    }
                }
            }
        }
    }

    cli_uninit(&cli);
    cli_server_uninit(&server);

    dprint("fuck you");

    return 0;
}
