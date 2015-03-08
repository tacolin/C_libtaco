#ifndef _TREE_H_
#define _TREE_H_

#include "basic.h"
#include "list.h"

////////////////////////////////////////////////////////////////////////////////

#define tTreeNodeCleanFunc tListContentCleanFn

////////////////////////////////////////////////////////////////////////////////

typedef enum 
{
    TREE_OK = 0,
    TREE_ERROR = -1

} tTreeStatus;

typedef void (*tTreeNodePrintFunc)(void* node, int layer);

typedef struct tTree 
{
    int                is_init;
    tList              nodes;
    tTreeNodePrintFunc print_fn;

} tTree;

typedef struct tTreeHdr 
{
    int guard_code;
    int is_init;

    struct tTreeHdr* parent;
    tList            childs;

    tTree* tree;

} tTreeHdr;


////////////////////////////////////////////////////////////////////////////////

tTreeStatus tree_init(tTree* tree, void* root, tTreeNodeCleanFunc clean_fn, tTreeNodePrintFunc print_fn);
tTreeStatus tree_clean(tTree* tree);

tTreeStatus tree_add(void* parent, void* child);
tTreeStatus tree_remove(void* parent, void* child);

tTreeStatus tree_isLeaf(void* node);

tTreeStatus tree_isAncestor(void* descendant, void* ancestor);
void* tree_commonAncestor(void* node1, void* node2);
tTreeStatus tree_print(tTree* tree);

tTreeStatus tree_route(void* src, void* dst, tList* route);

#endif //_TREE_H_
