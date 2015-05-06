#ifndef _FAST_QUEUE_H_
#define _FAST_QUEUE_H_

#define FQUEUE_OK (0)
#define FQUEUE_FAIL (-1)

#define FQUEUE_FOREACH(pfqueue, _data) for (_data = fqueue_pop(pfqueue); _data; _data = fqueue_pop(pfqueue))

struct fqueue;

struct fqueue* fqueue_create(void (*cleanfn)(void* ud));
void fqueue_release(struct fqueue* fq);

int   fqueue_push(struct fqueue* fq, void* ud);
void* fqueue_pop(struct fqueue* fq);

int fqueue_empty(struct fqueue* fq);

#endif //_FAST_QUEUE_H_
