#include <stdint.h>

#include "basic.h"
#include "list.h"

static int _larger_than(void* data, void* arg)
{
    intptr_t num = (intptr_t)data;
    intptr_t cmp = (intptr_t)arg;
    return (num > cmp) ? 1 : 0 ;
}

int main(int argc, char const *argv[])
{
    struct list list;
    void* node;
    void* data;
    list_init(&list, NULL);

    {
        list_append(&list, (void*)1);
        list_append(&list, (void*)2);
        list_append(&list, (void*)3);
        list_append(&list, (void*)4);
        list_append(&list, (void*)5);

        LIST_FOREACH(&list, node, data)
        {
            dprint("append data = %p", data);
        }
    }
    list_clean(&list);

    {
        list_insert(&list, (void*)10);
        list_insert(&list, (void*)20);
        list_insert(&list, (void*)30);

        LIST_FOREACH(&list, node, data)
        {
            dprint("insert data = %p", data);
        }
    }
    list_clean(&list);

    {
        list_insert(&list, (void*)10);
        list_insert(&list, (void*)20);
        list_insert(&list, (void*)30);

        list_remove(&list, (void*)20);

        LIST_FOREACH(&list, node, data)
        {
            dprint("rest insert data = %p", data);
        }
    }
    list_clean(&list);

    {
        list_insert(&list, (void*)10);
        list_insert(&list, (void*)20);
        list_insert(&list, (void*)30);

        data = list_tail(&list);
        dprint("tail data = %p", data);

        data = list_find(&list, _larger_than, (void*)20);
        dprint("_larger_than 20 data = %p", data);
    }
    list_clean(&list);

    {
        list_append(&list, (void*)10);
        list_append_after(&list, (void*)10, (void*)30);
        list_insert_before(&list, (void*)10, (void*)20);

        LIST_FOREACH(&list, node, data)
        {
            dprint("append after and insert before data = %p", data);
        }
    }
    list_clean(&list);

    dprint("ok");

    return 0;
}
