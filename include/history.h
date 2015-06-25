#ifndef _HISTORY_H_
#define _HISTORY_H_

#define HIS_OK (0)
#define HIS_FAIL (-1)

struct history;

struct history* history_create(int data_size, int max_num);
void history_release(struct history* h);

int history_add(struct history* h, void* data, int data_size);
int history_addstr(struct history* h, char* str);

int history_do(struct history* h, int start, int end, void (*execfn)(int idx, void* data, void* arg), void* arg);
int history_do_all(struct history* h, void (*execfn)(int idx, void* data, void* arg), void* arg);

void* history_get(struct history* h, int idx);
int history_num(struct history* h);

int history_clear(struct history* h);

#endif //_HISTORY_H_
