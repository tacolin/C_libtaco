#include "basic.h"
#include "logger.h"
#include "thread.h"
#include "queue.h"

static void _logger(void* arg)
{
    struct logger* lg = (struct logger*)arg;
    logger_run(lg);
}

static void _printer(void* arg)
{
    struct logger* lg = (struct logger*)arg;
    int i;
    for (i=0; i<10; i++)
    {
        log_print(lg, LOG_LV_INFO, "hello world INFO %d\n", i);
        log_print(lg, LOG_LV_V1,   "hello world V1 %d\n", i);
        sleep(1);

        if (i == 3)
        {
            logger_unset_file(lg);
        }

        if (i == 3)
        {
            logger_unset_udp(lg);
        }
    }
    logger_break(lg);
}

int main(int argc, char const *argv[])
{
//    struct logger* lg = logger_create(LOG_FLAG_UDP | LOG_FLAG_FILE | LOG_FLAG_STDOUT, "127.0.0.1", 55555, "taco.txt");
    struct logger* lg = logger_create(0, NULL, 0, NULL);

    logger_set_udp(lg, "127.0.0.1", 55555);
    logger_set_file(lg, "taco.txt");
    logger_set_stdout(lg);

    struct thread t[2] = {
        {_logger, lg},
        {_printer, lg}
    };

    thread_join(t, 2);

    logger_release(lg);
    dprint("ok");
    return 0;
}
