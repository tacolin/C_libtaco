#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ringbuf.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static void _increase_rb(struct ringbuf* rb, int* pos, int* msb)
{
    *pos = *pos + 1;
    if (*pos == rb->size)
    {
        *msb ^= 1;
        *pos = 0;
    }
}

int ringbuf_full(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return 1, "rb is null");
    CHECK_IF(rb->size <= 0, return 1, "rb_size = %d invalid", rb->size);
    return (rb->end == rb->start) && (rb->e_msb != rb->s_msb);
}

int ringbuf_empty(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return 1, "rb is null");
    CHECK_IF(rb->size <= 0, return 1, "rb_size = %d invalid", rb->size);
    return (rb->end == rb->start) && (rb->e_msb == rb->s_msb);
}

int ringbuf_init(struct ringbuf* rb, int rb_size, int elem_size, void* elements)
{
    CHECK_IF(rb == NULL, return RB_FAIL, "rb is null");
    CHECK_IF(rb_size <= 0, return RB_FAIL, "rb_size = %d invalid", rb_size);
    CHECK_IF(elem_size <= 0, return RB_FAIL, "elem_size = %d invalid", elem_size);

    rb->size  = rb_size;
    rb->start = 0;
    rb->end   = 0;
    rb->s_msb = 0;
    rb->e_msb = 0;
    if (elements)
    {
        rb->elements = elements;
        rb->is_need_free = 0;
    }
    else
    {
        rb->elements = calloc(elem_size, rb_size);
        CHECK_IF(rb->elements == NULL, return RB_FAIL, "calloc failed");
        rb->is_need_free = 1;
    }
    rb->elem_size = elem_size;
    return RB_OK;
}

int ringbuf_uninit(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return RB_FAIL, "rb is null");

    if (rb->elements && rb->is_need_free)
    {
        free(rb->elements);
    }
    memset(rb, 0, sizeof(struct ringbuf));
    return RB_OK;
}

void* ringbuf_pre_write(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return NULL, "rb is null");
    CHECK_IF(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    CHECK_IF(rb->elements == NULL, return NULL, "rb->elements is null");
    return rb->elements + rb->elem_size * rb->end;
}

int ringbuf_post_write(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return RB_FAIL, "rb is null");
    CHECK_IF(rb->size <= 0, return RB_FAIL, "rb->size = %d invalid", rb->size);
    CHECK_IF(rb->elements == NULL, return RB_FAIL, "rb->elements is null");

    if (ringbuf_full(rb)) _increase_rb(rb, &rb->start, &rb->s_msb);

    _increase_rb(rb, &rb->end, &rb->e_msb);
    return RB_OK;
}

int ringbuf_write(struct ringbuf* rb, void* input)
{
    CHECK_IF(input == NULL, return RB_FAIL, "input is null");
    // ringbuf_pre_write will do the rest input checks

    void* elem = ringbuf_pre_write(rb);
    if (elem == NULL) return RB_FAIL;

    memcpy(elem, input, rb->elem_size);
    return ringbuf_post_write(rb);
}

void* ringbuf_pre_read(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return NULL, "rb is null");
    CHECK_IF(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    CHECK_IF(rb->elements == NULL, return NULL, "rb->elements is null");

    if (ringbuf_empty(rb)) return NULL;

    return rb->elements + rb->elem_size * rb->start;
}

int ringbuf_post_read(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return RB_FAIL, "rb is null");
    CHECK_IF(rb->size <= 0, return RB_FAIL, "rb->size = %d invalid", rb->size);
    CHECK_IF(rb->elements == NULL, return RB_FAIL, "rb->elements is null");

    if (ringbuf_empty(rb)) return RB_FAIL;

    _increase_rb(rb, &rb->start, &rb->s_msb);
    return RB_OK;
}

int ringbuf_read(struct ringbuf* rb, void* output)
{
    CHECK_IF(output == NULL, return RB_FAIL, "output is null");
    // ringbuf_pre_read will do the rest input checks

    void* elem = ringbuf_pre_read(rb);
    if (elem == NULL) return RB_FAIL;

    memcpy(output, elem, rb->elem_size);
    return ringbuf_post_read(rb);
}

void* ringbuf_tail(struct ringbuf* rb)
{
    CHECK_IF(rb == NULL, return NULL, "rb is null");
    CHECK_IF(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    CHECK_IF(rb->elements == NULL, return NULL, "rb->elements is null");

    if (rb->end == 0) rb->elements + rb->elem_size * (rb->size - 1);

    return rb->elements + rb->elem_size * rb->end;
}

void* ringbuf_prev(struct ringbuf* rb, void* elem)
{
    CHECK_IF(rb == NULL, return NULL, "rb is null");
    CHECK_IF(elem == NULL, return NULL, "elem is null");
    CHECK_IF(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    CHECK_IF(rb->elements == NULL, return NULL, "rb->elements is null");

    if (rb->elements == elem) return rb->elements + rb->elem_size * (rb->size - 1);

    return elem - rb->elem_size;
}
