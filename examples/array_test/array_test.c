#include <stdint.h>

#include "basic.h"
#include "array.h"

int main(int argc, char const *argv[])
{
    struct array array = {};

    array_init(&array, NULL);

    array_add(&array, (void*)(intptr_t)1);
    array_add(&array, (void*)(intptr_t)2);
    array_add(&array, (void*)(intptr_t)3);
    array_add(&array, (void*)(intptr_t)4);
    array_add(&array, (void*)(intptr_t)5);

    int i;
    for (i=0; i<array.num; i++)
    {
        dprint("array->datas[%d] = %d", i, (intptr_t)array.datas[i]);
    }

    array_uninit(&array);

    dprint("ok");
    return 0;
}
