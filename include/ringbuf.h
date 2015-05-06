#ifndef _RING_BUF_H_
#define _RING_BUF_H_

// ring buffer (circuluar buffer) mirror implementation

#define RB_OK (0)
#define RB_FAIL (-1)

struct ringbuf
{
    int size;
    int start;
    int end;
    int s_msb;
    int e_msb;

    void* elements;
    int elem_size;

    int is_need_free;
};

int ringbuf_init(struct ringbuf* rb, int rb_size, int elem_size, void* elements);
int ringbuf_uninit(struct ringbuf* rb);

int ringbuf_write(struct ringbuf* rb, void* input);
int ringbuf_read(struct ringbuf* rb, void* output);

int ringbuf_empty(struct ringbuf* rb); // yes : return 1(true), no return 0(false)
int ringbuf_full(struct ringbuf* rb); // yes : return 1(true), no return 0(false)

void* ringbuf_pre_read(struct ringbuf* rb);
int ringbuf_post_read(struct ringbuf* rb);

void* ringbuf_pre_write(struct ringbuf* rb);
int ringbuf_post_write(struct ringbuf* rb);

void* ringbuf_tail(struct ringbuf* rb);
void* ringbuf_prev(struct ringbuf* rb, void* elem);

#endif //_RING_BUF_H_
