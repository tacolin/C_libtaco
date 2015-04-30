#ifndef _RING_BUF_H_
#define _RING_BUF_H_

// ring buffer (circuluar buffer) mirror implementation

#include "basic.h"

////////////////////////////////////////////////////////////////////////////////

#define RB_OK (0)
#define RB_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef struct tRingBuf
{
    int size;
    int start;
    int end;
    int s_msb;
    int e_msb;

    void* elements;
    int elem_size;

    int is_need_free;

} tRingBuf;

////////////////////////////////////////////////////////////////////////////////

int ringbuf_init(tRingBuf* rb, int rb_size, int elem_size, void* elements);
int ringbuf_uninit(tRingBuf* rb);

int ringbuf_write(tRingBuf* rb, void* input);
int ringbuf_read(tRingBuf* rb, void* output);

int ringbuf_isEmpty(tRingBuf* rb); // yes : return 1(true), no return 0(false)
int ringbuf_isFull(tRingBuf* rb); // yes : return 1(true), no return 0(false)

void* ringbuf_preRead(tRingBuf* rb);
int ringbuf_postRead(tRingBuf* rb);

void* ringbuf_preWrite(tRingBuf* rb);
int ringbuf_postWrite(tRingBuf* rb);

void* ringbuf_tail(tRingBuf* rb);
void* ringbuf_prev(tRingBuf* rb, void* elem);

#endif //_RING_BUF_H_
