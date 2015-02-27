#include "basic.h"
#include "list.h"
#include "queue.h"

tListBool largerThan(void* content, void* arg)
{
    long num = (long)arg;
    long get = (long)content;

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

    get = list_head(&list);
    while (get)
    {
        dprint("get = %ld", *get);
        get = list_next(&list, get);
    }

    list_remove(&list, &b);

    get = list_head(&list);
    while (get)
    {
        dprint("get = %ld", *get);
        get = list_next(&list, get);
    }

    get = list_tail(&list);
    dprint("get = %ld", *get);

    get = list_find(&list, largerThan, (void*)20);
    dprint("get = %ld", *get);

    list_clean(&list);

    dprint("ok");

    return 0;
}