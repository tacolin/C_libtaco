#ifndef _TREE_H_
#define _TREE_H_

#include "list.h"

#define TREE_OK (0)
#define TREE_FAIL (-1)

struct tree
{
    int is_init;
    struct list nodes;
};

struct tree_hdr
{
    struct tree_hdr* parent;
    struct list      childs;

    int layer;
    struct tree* tree;

    int guard_code;
    int is_init;
};

int tree_init(struct tree* tree, void* root, void (*cleanfn)(void* node));
int tree_clean(struct tree* tree);

int tree_add(void* parent, void* child);
int tree_remove(void* parent, void* child);

int tree_is_leaf(void* node);

int tree_is_ancestor(void* descendant, void* ancestor);
void* tree_common_ancestor(void* node1, void* node2);

int tree_route(void* src, void* dst, struct list* route);

int tree_dfs(struct tree* tree, void (*execfn)(void* node, int layer));
int tree_bfs(struct tree* tree, void (*execfn)(void* node, int layer));

struct list* tree_childs(void* node);
void* tree_parent(void* node);

#endif //_TREE_H_
