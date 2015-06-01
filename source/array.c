#include <stdio.h>
#include <stdlib.h>

#include "array.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

int array_init(struct array* array, void (*cleanfn)(void* data))
{
    CHECK_IF(array == NULL, return ARRAY_FAIL, "array is null");

    array->num     = 0;
    array->datas   = NULL;
    array->cleanfn = cleanfn;
    return ARRAY_OK;
}

void array_uninit(struct array* array)
{
    CHECK_IF(array == NULL, return, "array is null");

    if (array->num == 0) return;

    CHECK_IF(array->datas == NULL, return, "array->num = %d but datas is null", array->num);

    if (array->cleanfn)
    {
        int i;
        for (i=0; i<array->num; i++) array->cleanfn(array->datas[i]);
    }
    free(array->datas);
    array->datas = NULL;
    return;
}

int array_add(struct array* array, void* data)
{
    CHECK_IF(array == NULL, return ARRAY_FAIL, "array is null");
    CHECK_IF(data == NULL, return ARRAY_FAIL, "data is null");

    array->num++;
    array->datas = realloc(array->datas, sizeof(void*) * array->num);
    array->datas[array->num-1] = data;
    return ARRAY_OK;
}

int array_find(struct array* array, int (*findfn)(void* data, void* arg), void* arg)
{
    CHECK_IF(array == NULL, return -1, "array is null");

    if (array->num == 0) return -1;

    int i;
    for (i=0; i<array->num; i++)
    {
        if (findfn)
        {
            if (findfn(array->datas[i], arg)) return i;
        }
        else
        {
            if (array->datas[i] == arg) return i;
        }
    }
    return -1;
}
