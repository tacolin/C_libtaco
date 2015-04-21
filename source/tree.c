#include <string.h>

#include "tree.h"

////////////////////////////////////////////////////////////////////////////////

#define TREE_GURAD_CODE 0x55665566

////////////////////////////////////////////////////////////////////////////////

static tTreeStatus _dfs(void* node, tTreeNodeExecFunc exec_fn)
{
    check_if(node == NULL, return TREE_ERROR, "node is null");
    check_if(exec_fn == NULL, return TREE_ERROR, "exec_fn is null");

    tTreeHdr* hdr = (tTreeHdr*)node;

    if (exec_fn)
    {
        exec_fn(node, hdr->layer);
    }

    tListObj* obj;
    void* child;
    LIST_FOREACH(&hdr->childs, obj, child)
    {
        _dfs(child, exec_fn);
    }

    return TREE_OK;
}

static tTreeStatus _isNodeClean(void* node)
{
    check_if(node == NULL, return TREE_ERROR, "node is null");

    tTreeHdr* hdr = (tTreeHdr*)node;

    if (hdr->is_init && hdr->guard_code == TREE_GURAD_CODE)
    {
        return TREE_ERROR;
    }

    return TREE_OK;
}

tTreeStatus _initTreeHdr(void* node, void* parent, tTree* tree)
{
    check_if(node == NULL, return TREE_ERROR, "node is null");
    check_if(tree == NULL, return TREE_ERROR, "tree is null");
    check_if(_isNodeClean(node) != TREE_OK, return TREE_ERROR, "_isNodeClean failed");

    tTreeHdr* hdr = (tTreeHdr*)node;
    hdr->parent = parent;
    hdr->tree   = tree;

    list_init(&hdr->childs, NULL);

    hdr->layer      = 0;
    hdr->guard_code = TREE_GURAD_CODE;
    hdr->is_init    = 1;

    return TREE_OK;
}

tTreeStatus _uninitTreeHdr(void* node)
{
    check_if(node == NULL, return TREE_ERROR, "node is null");
    tTreeHdr* hdr = (tTreeHdr*)node;

    check_if(hdr->is_init != 1, return TREE_ERROR, "node is not init yet");
    check_if(hdr->guard_code != TREE_GURAD_CODE, return TREE_ERROR, "guard code is error");

    list_clean(&hdr->childs);
    hdr->tree       = NULL;
    hdr->parent     = NULL;
    hdr->guard_code = 0;
    hdr->is_init    = 0;
    hdr->layer      = 0;

    return TREE_OK;
}

static tListBool _isLayerBiggerThanMe(void* target, void* me)
{
    check_if(target == NULL, return LIST_FALSE, "target is null");
    check_if(me == NULL, return LIST_FALSE, "me is null");

    tTreeHdr* node = (tTreeHdr*)target;
    tTreeHdr* mynode = (tTreeHdr*)me;

    return (node->layer > mynode->layer) ? LIST_TRUE : LIST_FALSE;
}

////////////////////////////////////////////////////////////////////////////////

tTreeStatus tree_init(tTree* tree, void* root, tTreeNodeCleanFunc clean_fn)
{
    check_if(tree == NULL, return TREE_ERROR, "tree is null");
    check_if(root == NULL, return TREE_ERROR, "root is null");

    memset(tree, 0, sizeof(tTree));

    tListStatus list_ret = list_init(&tree->nodes, clean_fn);
    check_if(list_ret != LIST_OK, return TREE_ERROR, "list_init failed");

    tTreeStatus tree_ret = _initTreeHdr(root, NULL, tree);
    check_if(tree_ret != TREE_OK, return TREE_ERROR, "_initTreeHdr failed");

    list_ret = list_append(&tree->nodes, root);
    check_if(list_ret != LIST_OK, return TREE_ERROR, "list_append root failed");

    tree->is_init  = 1;

    return TREE_OK;
}

tTreeStatus tree_clean(tTree* tree)
{
    check_if(tree == NULL, return TREE_ERROR, "tree is null");
    check_if(tree->is_init != 1, return TREE_ERROR, "tree is not init yet");

    void* node;
    tListObj* obj;
    LIST_FOREACH(&tree->nodes, obj, node)
    {
        _uninitTreeHdr(node);
    }

    list_clean(&tree->nodes);

    tree->is_init = 0;

    return TREE_OK;
}

tTreeStatus tree_add(void* parent, void* child)
{
    check_if(parent == NULL, return TREE_ERROR, "parent is null");
    check_if(child == NULL, return TREE_ERROR, "child is null");
    check_if(parent == child, return TREE_ERROR, "parent = child");
    check_if(_isNodeClean(child) != TREE_OK, return TREE_ERROR, "child is already in some tree");

    tTreeHdr* parent_hdr = (tTreeHdr*)parent;
    tTree* tree          = parent_hdr->tree;

    tTreeStatus ret = _initTreeHdr(child, parent, tree);
    check_if(ret != TREE_OK, return TREE_ERROR, "_initTreeHdr failed");

    tTreeHdr* child_hdr  = (tTreeHdr*)child;
    child_hdr->layer = parent_hdr->layer + 1;

    tListStatus list_ret = list_append(&parent_hdr->childs, child);
    check_if(list_ret != LIST_OK, goto _ERROR, "list_append to parent failed");

    tListObj* target_obj = list_findObj(&tree->nodes, _isLayerBiggerThanMe, child);
    if (target_obj == NULL)
    {
        target_obj = list_tailObj(&tree->nodes);
        if (target_obj == NULL)
        {
            list_ret = list_append(&tree->nodes, child);
            check_if(list_ret != LIST_OK, goto _ERROR, "list_append to tree failed");
        }
        else
        {
            list_ret = list_appendToObj(&tree->nodes, target_obj, child);
            check_if(list_ret != LIST_OK, goto _ERROR, "list_appendTo failed");
        }
    }
    else
    {
        list_ret = list_insertToObj(&tree->nodes, target_obj, child);
        check_if(list_ret != LIST_OK, goto _ERROR, "list_insertTo failed");
    }

    return TREE_OK;

_ERROR:
    list_remove(&parent_hdr->childs, child);
    list_remove(&tree->nodes, child);
    _uninitTreeHdr(child);
    return TREE_ERROR;
}

tTreeStatus tree_remove(void* parent, void* child)
{
    check_if(parent == NULL, return TREE_ERROR, "parent is null");
    check_if(child == NULL, return TREE_ERROR, "child is null");
    check_if(parent == child, return TREE_ERROR, "parent = child");
    check_if(_isNodeClean(child) == TREE_OK, return TREE_ERROR, "child is not in any tree");
    check_if(tree_isLeaf(child) != TREE_OK, return TREE_ERROR, "child is not a leaf");

    tTreeHdr* parent_hdr = (tTreeHdr*)parent;
    tTree* tree          = parent_hdr->tree;
    tListStatus list_ret;

    list_ret = list_remove(&parent_hdr->childs, child);
    check_if(list_ret != LIST_OK, return TREE_ERROR, "child is not born by parent");

    list_ret = list_remove(&tree->nodes, child);
    check_if(list_ret != LIST_OK, return TREE_ERROR, "child is not in this tree");

    return _uninitTreeHdr(child);
}

tTreeStatus tree_isLeaf(void* node)
{
    check_if(node == NULL, return TREE_ERROR, "node is null");
    check_if(_isNodeClean(node) == TREE_OK, return TREE_ERROR, "node is not added to any tree");

    tTreeHdr* hdr = (tTreeHdr*)node;

    if (hdr->parent == NULL) return TREE_ERROR; // root
    if (list_length(&hdr->childs) > 0) return TREE_ERROR; // node has children

    return TREE_OK;
}

tTreeStatus tree_isAncestor(void* descendant, void* ancestor)
{
    check_if(descendant == NULL, return TREE_ERROR, "descendant is null");
    check_if(ancestor == NULL, return TREE_ERROR, "ancestor is null");
    check_if(descendant == ancestor, return TREE_ERROR, "descendant = ancestor");

    tTreeHdr* des = (tTreeHdr*)descendant;
    tTreeHdr* anc = (tTreeHdr*)ancestor;
    tTreeHdr* node;
    for (node = des->parent; node; node = node->parent)
    {
        if (node == anc)
        {
            return TREE_OK;
        }
    }
    return TREE_ERROR;
}

void* tree_commonAncestor(void* node1, void* node2)
{
    check_if(node1 == NULL, return NULL, "node1 is null");
    check_if(node2 == NULL, return NULL, "node2 is null");
    check_if(node1 == node2, return NULL, "node1 = node2");

    if (TREE_OK == tree_isAncestor(node1, node2))
    {
        return node2;
    }
    else if (TREE_OK == tree_isAncestor(node2, node1))
    {
        return node1;
    }
    else
    {
        tTreeHdr* hdr1 = (tTreeHdr*)node1;
        tTreeHdr* ancestor;
        for (ancestor = hdr1->parent; ancestor; ancestor = ancestor->parent)
        {
            if (TREE_OK == tree_isAncestor(node2, ancestor))
            {
                return ancestor;
            }
        }
    }

    return NULL;
}

tTreeStatus tree_route(void* src, void* dst, tList* route)
{
    check_if(src == NULL, return TREE_ERROR, "src is null");
    check_if(dst == NULL, return TREE_ERROR, "dst is null");
    check_if(src == dst, return TREE_ERROR, "src = dst");
    check_if(route == NULL, return TREE_ERROR, "route is null");
    check_if(_isNodeClean(src) == TREE_OK, return TREE_ERROR, "src is not add to tree.");
    check_if(_isNodeClean(dst) == TREE_OK, return TREE_ERROR, "dst is not add to tree.");

    tTreeHdr* srchdr = (tTreeHdr*)src;
    tTreeHdr* dsthdr = (tTreeHdr*)dst;
    check_if(srchdr->tree != dsthdr->tree, return TREE_ERROR, "src and dst are in diff tree.");

    list_clean(route);

    void* cmn_ancestor = tree_commonAncestor(src, dst);
    check_if(cmn_ancestor == NULL, return TREE_ERROR, "no common ancestor");

    tListStatus ret;
    tTreeHdr* node;
    for (node = src; node != cmn_ancestor; node = node->parent)
    {
        ret = list_append(route, node);
        check_if(ret != LIST_OK, goto _ERROR, "record src to ancestor failed");
    }

    ret = list_append(route, cmn_ancestor);
    check_if(ret != LIST_OK, goto _ERROR, "record ancestor failed");

    for (node = dst; node != cmn_ancestor; node = node->parent)
    {
        ret = list_appendTo(route, cmn_ancestor, node);
        check_if(ret != LIST_OK, goto _ERROR, "record ancestor to dst failed");
    }

    return TREE_OK;

_ERROR:
    list_clean(route);
    return TREE_ERROR;
}

tTreeStatus tree_dfs(tTree* tree, tTreeNodeExecFunc exec_fn)
{
    check_if(tree == NULL, return TREE_ERROR, "tree is null");
    check_if(exec_fn == NULL, return TREE_ERROR, "exec_fn is null");
    check_if(tree->is_init != 1, return TREE_ERROR, "tree is not init yet");

    void* root = list_head(&tree->nodes);
    check_if(root == NULL, return TREE_ERROR, "tree is empty");

    return _dfs(root, exec_fn);
}

tTreeStatus tree_bfs(tTree* tree, tTreeNodeExecFunc exec_fn)
{
    check_if(tree == NULL, return TREE_ERROR, "tree is null");
    check_if(exec_fn == NULL, return TREE_ERROR, "exec_fn is null");
    check_if(tree->is_init != 1, return TREE_ERROR, "tree is not init yet");

    void* node;
    tListObj* obj;
    LIST_FOREACH(&tree->nodes, obj, node)
    {
        exec_fn(node, ((tTreeHdr*)node)->layer);
    }

    return TREE_OK;
}

tList* tree_getChilds(void* node)
{
    check_if(node == NULL, return NULL, "node is null");
    check_if(_isNodeClean(node) == TREE_OK, return NULL, "node is not added to any tree");

    tTreeHdr* hdr = (tTreeHdr*)node;
    return &(hdr->childs);
}
