#ifndef _TREE_H_
#define _TREE_H_

#include "basic.h"
#include "list.h"

////////////////////////////////////////////////////////////////////////////////

#define tTreeNodeCleanFunc tListContentCleanFn

#define TREE_OK (0)
#define TREE_FAIL (-1)

////////////////////////////////////////////////////////////////////////////////

typedef void (*tTreeNodeExecFunc)(void* node, int layer);

typedef struct tTree
{
    int   is_init;
    tList nodes;  // bfs list

} tTree;

typedef struct tTreeHdr
{
    int guard_code;
    int is_init;

    struct tTreeHdr* parent;
    tList            childs;

    int layer;
    tTree* tree;

} tTreeHdr;


////////////////////////////////////////////////////////////////////////////////

int tree_init(tTree* tree, void* root, tTreeNodeCleanFunc clean_fn);
int tree_clean(tTree* tree);

int tree_add(void* parent, void* child);
int tree_remove(void* parent, void* child);

int tree_isLeaf(void* node);

int tree_isAncestor(void* descendant, void* ancestor);
void* tree_commonAncestor(void* node1, void* node2);

int tree_route(void* src, void* dst, tList* route);

int tree_dfs(tTree* tree, tTreeNodeExecFunc exec_fn);
int tree_bfs(tTree* tree, tTreeNodeExecFunc exec_fn);

tList* tree_childs(void* node);
void* tree_parent(void* node);

#endif //_TREE_H_
