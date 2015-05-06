#include <stdint.h>

#include "basic.h"
#include "fast_list.h"

struct taco
{
    struct flist_hdr hdr;
    int value;
};

static int _larger_than(void* data, void* arg)
{
    struct taco* t = (struct taco*)data;
    intptr_t a = (intptr_t)arg;
    return (t->value > a) ? 1 : 0 ;
}

int main(int argc, char const *argv[])
{
    struct flist list;
    int i;

    flist_init(&list, NULL);
    struct taco t[5] = {0};
    struct taco* get;

    {
        for (i=0; i<5; i++)
        {
            t[i].value = i+1;
            flist_append(&list, &t[i]);
        }

        FLIST_FOREACH(&list, get)
        {
            dprint("append data = %d", get->value);
        }
    }
    flist_clean(&list);

    {
        for (i=0; i<3; i++)
        {
            t[i].value = (i+1)*10;
            flist_insert(&list, &t[i]);
        }

        FLIST_FOREACH(&list, get)
        {
            dprint("insert data = %d", get->value);
        }
    }
    flist_clean(&list);

    {
        for (i=0; i<3; i++)
        {
            t[i].value = (i+1)*10;
            flist_insert(&list, &t[i]);
        }

        flist_remove(&list, &t[1]);

        FLIST_FOREACH(&list, get)
        {
            dprint("rest insert data = %d", get->value);
        }
    }
    flist_clean(&list);

    {
        for (i=0; i<3; i++)
        {
            t[i].value = (i+1)*10;
            flist_insert(&list, &t[i]);
        }

        get = flist_tail(&list);
        dprint("tail data = %d", get->value);

        get = flist_find(&list, _larger_than, (void*)20);
        dprint("_larger_than 20 data = %d", get->value);
    }
    flist_clean(&list);

    {
        for (i=0; i<3; i++)
        {
            t[i].value = (i+1)*10;
        }

        flist_append(&list, &t[0]);
        flist_append_after(&list, &t[0], &t[2]);
        flist_insert_before(&list, &t[0], &t[1]);

        FLIST_FOREACH(&list, get)
        {
            dprint("append after and insert before data = %d", get->value);
        }
    }
    flist_clean(&list);

    dprint("ok");

    return 0;
}
