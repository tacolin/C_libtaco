#ifndef _LIST_H_
#define _LIST_H_

#define LIST_OK   (0)
#define LIST_FAIL (-1)

#define LIST_FOREACH(plist, _node, _data) \
    for (_node = list_head_node(plist), _data = (_node) ? ((struct list_node*)_node)->data : NULL; \
         _node && _data; \
         _node = list_next_node((struct list_node*)_node), _data = (_node) ? ((struct list_node*)_node)->data : NULL)

struct list_node
{
    struct list_node* prev;
    struct list_node* next;
    void* data;
};

struct list
{
    struct list_node* head;
    struct list_node* tail;
    int num;
    void (*cleanfn)(void* data);
};

int list_init(struct list* list, void (*cleanfn)(void*));
void list_clean(struct list* list);

int list_append(struct list* list, void* data);
int list_insert(struct list* list, void* data);

struct list_node* list_find_node(struct list* list, int (*findfn)(void* data, void* arg), void* arg);
int list_remove_node(struct list* list, struct list_node* node);

void* list_find(struct list* list, int (*findfn)(void* data, void* arg), void* arg);
int list_remove(struct list* list, void* data);

struct list_node* list_head_node(struct list* list);
struct list_node* list_tail_node(struct list* list);

void* list_head(struct list* list);
void* list_tail(struct list* list);

struct list_node* list_prev_node(struct list_node* node);
struct list_node* list_next_node(struct list_node* node);

void* list_prev(struct list* list, void* data);
void* list_next(struct list* list, void* data);

int list_append_after_node(struct list* list, struct list_node* target_node, void* data);
int list_insert_before_node(struct list* list, struct list_node* target_node, void* data);

int list_append_after(struct list* list, void* target, void* data);
int list_insert_before(struct list* list, void* target, void* data);

int list_num(struct list* list);

#endif //_LIST_H_
