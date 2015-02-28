#include "basic.h"
#include "timer.h"

typedef struct tTaco
{
    char name[20];

} tTaco;

static int _running = 1;

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    dprint("stop main()");
}

static void _expired(void* arg)
{
    tTaco* taco = (tTaco*)arg;
    dprint("%s time used = %.2lf", taco->name, (double)clock()/CLOCKS_PER_SEC);
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, _sigIntHandler);

    timer_system_init();

    tTaco data1 = {};
    tTaco data2 = {};
    tTaco data3 = {};

    snprintf(data1.name, 20, "data1");
    snprintf(data2.name, 20, "data2");
    snprintf(data3.name, 20, "data3");

    tTimer timer1;
    tTimer timer2;
    tTimer timer3;

    dprint("time used = %.2lf", (double)clock()/CLOCKS_PER_SEC);

    timer_init(&timer1, "TEST TIMER1", 500, _expired, &data1, TIMER_PERIODIC);
    timer_init(&timer2, "TEST TIMER2", 1000, _expired, &data2, TIMER_ONCE);
    timer_init(&timer3, "TEST TIMER3", 2000, _expired, &data3, TIMER_ONCE);

    dprint("time used = %.2lf", (double)clock()/CLOCKS_PER_SEC);

    timer_start(&timer1);
    timer_start(&timer2);
    timer_start(&timer3);

    while (_running);

    timer_system_uninit();

    return 0;
}
