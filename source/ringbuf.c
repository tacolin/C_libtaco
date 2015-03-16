#include "ringbuf.h"
#include <string.h>

static void _increaseRb(tRingBuf* rb, int* pos, int* msb)
{
    *pos = *pos + 1;
    if (*pos == rb->size)
    {
        *msb ^= 1;
        *pos = 0;
    }
}

int ringbuf_isFull(tRingBuf* rb)
{
    check_if(rb == NULL, return 1, "rb is null");
    check_if(rb->size <= 0, return 1, "rb_size = %d invalid", rb->size);

    return (rb->end == rb->start) && (rb->e_msb != rb->s_msb);
}

int ringbuf_isEmpty(tRingBuf* rb)
{
    check_if(rb == NULL, return 1, "rb is null");
    check_if(rb->size <= 0, return 1, "rb_size = %d invalid", rb->size);

    return (rb->end == rb->start) && (rb->e_msb == rb->s_msb);
}

tRbStatus ringbuf_init(tRingBuf* rb, int rb_size, int elem_size, void* elements)
{
    check_if(rb == NULL, return RB_ERROR, "rb is null");
    check_if(rb_size <= 0, return RB_ERROR, "rb_size = %d invalid", rb_size);
    check_if(elem_size <= 0, return RB_ERROR, "elem_size = %d invalid", elem_size);

    // memset(rb, 0, sizeof(tRingBuf));

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
        check_if(rb->elements == NULL, return RB_ERROR, "calloc failed");
        rb->is_need_free = 1;
    }

    rb->elem_size = elem_size;

    return RB_OK;
}

tRbStatus ringbuf_uninit(tRingBuf* rb)
{
    check_if(rb == NULL, return RB_ERROR, "rb is null");

    if (rb->elements && rb->is_need_free)
    {
        free(rb->elements);
    }

    memset(rb, 0, sizeof(tRingBuf));
    return RB_OK;
}

void* ringbuf_preWrite(tRingBuf* rb)
{
    check_if(rb == NULL, return NULL, "rb is null");
    check_if(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    check_if(rb->elements == NULL, return NULL, "rb->elements is null");

    return rb->elements + rb->elem_size * rb->end;
}

tRbStatus ringbuf_postWrite(tRingBuf* rb)
{
    check_if(rb == NULL, return RB_ERROR, "rb is null");
    check_if(rb->size <= 0, return RB_ERROR, "rb->size = %d invalid", rb->size);
    check_if(rb->elements == NULL, return RB_ERROR, "rb->elements is null");

    if (ringbuf_isFull(rb)) _increaseRb(rb, &rb->start, &rb->s_msb);

    _increaseRb(rb, &rb->end, &rb->e_msb);

    return RB_OK;
}

tRbStatus ringbuf_write(tRingBuf* rb, void* input)
{
    check_if(input == NULL, return RB_ERROR, "input is null");
    // ringbuf_preWrite will do the rest input checks

    void* elem = ringbuf_preWrite(rb);
    if (elem == NULL) return RB_ERROR;

    memcpy(elem, input, rb->elem_size);

    return ringbuf_postWrite(rb);
}

void* ringbuf_preRead(tRingBuf* rb)
{
    check_if(rb == NULL, return NULL, "rb is null");
    check_if(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    check_if(rb->elements == NULL, return NULL, "rb->elements is null");

    if (ringbuf_isEmpty(rb)) return NULL;

    return rb->elements + rb->elem_size * rb->start;
}

tRbStatus ringbuf_postRead(tRingBuf* rb)
{
    check_if(rb == NULL, return RB_ERROR, "rb is null");
    check_if(rb->size <= 0, return RB_ERROR, "rb->size = %d invalid", rb->size);
    check_if(rb->elements == NULL, return RB_ERROR, "rb->elements is null");

    if (ringbuf_isEmpty(rb)) return RB_ERROR;

    _increaseRb(rb, &rb->start, &rb->s_msb);

    return RB_OK;
}

tRbStatus ringbuf_read(tRingBuf* rb, void* output)
{
    check_if(output == NULL, return RB_ERROR, "output is null");
    // ringbuf_preRead will do the rest input checks

    void* elem = ringbuf_preRead(rb);
    if (elem == NULL) return RB_ERROR;

    memcpy(output, elem, rb->elem_size);

    return ringbuf_postRead(rb);
}

void* ringbuf_tail(tRingBuf* rb)
{
    check_if(rb == NULL, return NULL, "rb is null");
    check_if(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    check_if(rb->elements == NULL, return NULL, "rb->elements is null");

    if (rb->end == 0) rb->elements + rb->elem_size * (rb->size - 1);

    return rb->elements + rb->elem_size * rb->end;
}

void* ringbuf_prev(tRingBuf* rb, void* elem)
{
    check_if(rb == NULL, return NULL, "rb is null");
    check_if(elem == NULL, return NULL, "elem is null");
    check_if(rb->size <= 0, return NULL, "rb->size = %d invalid", rb->size);
    check_if(rb->elements == NULL, return NULL, "rb->elements is null");

    if (rb->elements == elem) return rb->elements + rb->elem_size * (rb->size - 1);

    return elem - rb->elem_size;
}
