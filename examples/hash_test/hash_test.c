#define _GNU_SOURCE

#include "basic.h"
#include "hash.h"

int main(int argc, char const *argv[])
{
    struct hash h = {};

    hash_init(&h, 5, NULL);

    hash_add(&h, "a", (void*)100);
    hash_add(&h, "b", (void*)200);
    hash_add(&h, "c", (void*)300);
    hash_add(&h, "c", (void*)400);
    hash_modify(&h, "c", (void*)400);
    hash_add(&h, "d", (void*)500);
    hash_add(&h, "e", (void*)600);
    hash_add(&h, "f", (void*)700);

    char* key;
    void* value = NULL;

    HASH_FOREACH(&h, key, value)
    {
        dprint("%s = %ld", key, (long)value);
    }

    value = hash_find(&h, "c");
    dprint("c = %ld", (long)value);

    hash_del(&h, "c", &value);
    dprint("delte c = %ld", (long)value);

    HASH_FOREACH(&h, key, value)
    {
        dprint("%s = %ld", key, (long)value);
    }

    hash_uninit(&h);

    dprint("another h");

    hash_init(&h, 5, free);

    asprintf((char**)&value, "100");
    hash_add(&h, "a", value);

    asprintf((char**)&value, "200");
    hash_add(&h, "b", value);

    asprintf((char**)&value, "300");
    hash_add(&h, "c", value);

    HASH_FOREACH(&h, key, value)
    {
        dprint("%s = %s", key, (char*)value);
    }

    hash_del(&h, "a", &value);
    dprint("delete a = %s", (char*)value);
    free(value);

    HASH_FOREACH(&h, key, value)
    {
        dprint("%s = %s", key, (char*)value);
    }

    dprint("a in h : %s", hash_contains(&h, "a") ? "YES" : "NO");
    dprint("b in h : %s", hash_contains(&h, "b") ? "YES" : "NO");
    dprint("c in h : %s", hash_contains(&h, "c") ? "YES" : "NO");

    hash_uninit(&h);

    dprint("ok");

    return 0;
}
