#ifndef _ARRAY_H_
#define _ARRAY_H_

#define ARRAY_OK (0)
#define ARRAY_FAIL (-1)

struct array
{
    void** datas;
    int num;

    void (*cleanfn)(void* data);
};

// int array_init(struct array* array, void (*cleanfn)(void* data));
void array_uninit(struct array* array);

int array_add(struct array* array, void* data);
int array_find(struct array* array, int (*findfn)(void* data, void* arg), void* arg);

#endif //_ARRAY_H_
