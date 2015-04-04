#include "hash.h"

int main(int argc, char const *argv[])
{
    tHashTable table = {};

    hash_init(&table, 5, NULL);

    hash_add(&table, "a", (void*)100);
    hash_add(&table, "b", (void*)200);
    hash_add(&table, "c", (void*)300);
    hash_add(&table, "c", (void*)400);
    hash_add(&table, "d", (void*)500);
    hash_add(&table, "e", (void*)600);
    hash_add(&table, "f", (void*)700);

    void* value = NULL;

    value = hash_find(&table, "a");
    dprint("a = %ld", (long)value);

    value = hash_find(&table, "b");
    dprint("b = %ld", (long)value);

    value = hash_find(&table, "c");
    dprint("c = %ld", (long)value);

    hash_del(&table, "c", &value);
    dprint("delte c = %ld", (long)value);

    char* key;
    HASH_FOREACH(&table, key, value)
    {
        dprint("%s = %ld", key, (long)value);
    }

    hash_uninit(&table);

    dprint("another table");

    hash_init(&table, 5, free);

    asprintf((char**)&value, "100");
    hash_add(&table, "a", value);

    asprintf((char**)&value, "200");
    hash_add(&table, "b", value);

    asprintf((char**)&value, "300");
    hash_add(&table, "c", value);

    HASH_FOREACH(&table, key, value)
    {
        dprint("%s = %s", key, (char*)value);
    }

    hash_del(&table, "a", &value);
    dprint("delete a = %s", (char*)value);
    free(value);

    HASH_FOREACH(&table, key, value)
    {
        dprint("%s = %s", key, (char*)value);
    }

    dprint("a in table : %s", hash_contains(&table, "a") ? "YES" : "NO");
    dprint("b in table : %s", hash_contains(&table, "b") ? "YES" : "NO");
    dprint("c in table : %s", hash_contains(&table, "c") ? "YES" : "NO");

    hash_uninit(&table);

    dprint("ok");

    return 0;
}
