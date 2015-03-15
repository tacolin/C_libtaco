#ifndef _RING_BUF_H_
#define _RING_BUF_H_

// ring buffer (circuluar buffer) mirror implementation

#include "basic.h"

typedef enum 
{
    RB_OK = 0,
    RB_ERROR = -1

} tRbStatus;

typedef void (*tRbWriteFn)(void* input, void* buf);
typedef void (*tRbReadFn)(void* buf, void* output);

typedef struct tRingBuf
{
    int size;
    int start;
    int end;
    int s_msb;
    int e_msb;

    void* elements;
    int elem_size;

    tRbWriteFn write_fn;
    tRbReadFn  read_fn;

} tRingBuf;

tRbStatus ringbuf_init(tRingBuf* rb, int rb_size, int elem_size, tRbReadFn read_fn, tRbWriteFn write_fn);
tRbStatus ringbuf_uninit(tRingBuf* rb);

tRbStatus ringbuf_write(tRingBuf* rb, void* input);
tRbStatus ringbuf_read(tRingBuf* rb, void* output);

int ringbuf_isEmpty(tRingBuf* rb); // yes : return 1(true), no return 0(false)
int ringbuf_isFull(tRingBuf* rb); // yes : return 1(true), no return 0(false)

void* ringbuf_preRead(tRingBuf* rb);
tRbStatus ringbuf_postRead(tRingBuf* rb);

void* ringbuf_preWrite(tRingBuf* rb);
tRbStatus ringbuf_postWrite(tRingBuf* rb);

#endif //_RING_BUF_H_