#ifndef _FAST_LIST_H_
#define _FAST_LIST_H_

#define FLIST_OK (0)
#define FLIST_FAIL (-1)

#define FLIST_FOREACH(pflist, _node) for (_node = flist_head(pflist); _node; _node = flist_next(pflist, _node))

struct flist_hdr
{
    struct flist_hdr* prev;
    struct flist_hdr* next;
    int enable;
    int guard_code;
};

struct flist
{
    struct flist_hdr* head;
    struct flist_hdr* tail;
    int num;
    void (*cleanfn)(void* node);
    int is_init;
};

int flist_init(struct flist* list, void (*cleanfn)(void*));
void flist_clean(struct flist* list);

int flist_append(struct flist* list, void* node);
int flist_insert(struct flist* list, void* node);

void* flist_find(struct flist* list, int (*findfn)(void* node, void* arg), void* arg);
int flist_remove(struct flist* list, void* node);

void* flist_head(struct flist* list);
void* flist_tail(struct flist* list);

void* flist_prev(struct flist* list, void* node);
void* flist_next(struct flist* list, void* node);

int flist_append_after(struct flist* list, void* target, void* node);
int flist_insert_before(struct flist* list, void* target, void* node);

int flist_num(struct flist* list);

#endif //_FAST_LIST_H_
