#include "basic.h"
#include "list.h"

tListBool largerThan(void* content, void* arg)
{
    long num = (long)arg;
    long get = *(long*)content;

    if (get > num)
    {
        return LIST_TRUE;
    }

    return LIST_FALSE;
}

int main(int argc, char const *argv[])
{
    tList list;

    list_init(&list, NULL);

    list_append(&list, (void*)1);
    list_append(&list, (void*)2);
    list_append(&list, (void*)3);
    list_append(&list, (void*)4);
    list_append(&list, (void*)5);

    dprint("show 1, 2, 3, 4, 5");

    long* get = list_head(&list);
    while (get)
    {
        dprint("get = %ld", (long)get);
        get = list_next(&list, get);
    }

    list_clean(&list);

    long a = 10;
    long b = 20;
    long c = 30;

    list_insert(&list, &a);
    list_insert(&list, &b);
    list_insert(&list, &c);

    dprint("show 30, 20, 10");

    for (get = list_head(&list); get; get = list_next(&list, get))
    {
        dprint("get = %ld", *get);
    }

    list_remove(&list, &b);

    dprint("show 30, 10");

    tListObj* obj;
    LIST_FOREACH(&list, obj, get)
    {
        dprint("get = %ld", *get);
    }

    dprint("show tail = 10");

    get = list_tail(&list);
    dprint("get = %ld", *get);

    dprint("show 30 > 20");

    get = list_find(&list, largerThan, (void*)20);
    dprint("get = %ld", *get);

    list_clean(&list);

    dprint("show 10, 20, 30 - double insert/append failed");
    list_append(&list, &a);
    list_append(&list, &c);
    list_appendTo(&list, &a, &b);
    list_insertTo(&list, &a, &b);

    LIST_FOREACH(&list, obj, get)
    {
        dprint("get = %ld", *get);
    }

    list_clean(&list);

    dprint("ok");

    return 0;
}
