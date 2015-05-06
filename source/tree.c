#include <stdio.h>
#include <string.h>

#include "tree.h"

#define TREE_GURAD_CODE 0x55665566

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static int _dfs(void* node, void (*execfn)(void* node, int layer))
{
    CHECK_IF(node == NULL, return TREE_FAIL, "node is null");
    CHECK_IF(execfn == NULL, return TREE_FAIL, "execfn is null");

    struct tree_hdr* hdr = (struct tree_hdr*)node;
    if (execfn)
    {
        execfn(node, hdr->layer);
    }

    void* obj;
    void* child;
    LIST_FOREACH(&hdr->childs, obj, child)
    {
        _dfs(child, execfn);
    }
    return TREE_OK;
}

static int _is_node_unused(void* node)
{
    CHECK_IF(node == NULL, return 0, "node is null");

    struct tree_hdr* hdr = (struct tree_hdr*)node;
    if (hdr->is_init && hdr->guard_code == TREE_GURAD_CODE) return 0;

    return 1;
}

static int _init_tree_hdr(void* node, void* parent, struct tree* tree)
{
    CHECK_IF(node == NULL, return TREE_FAIL, "node is null");
    CHECK_IF(tree == NULL, return TREE_FAIL, "tree is null");
    CHECK_IF(!_is_node_unused(node), return TREE_FAIL, "_is_node_unused failed");

    struct tree_hdr* hdr = (struct tree_hdr*)node;
    hdr->parent          = parent;
    hdr->tree            = tree;
    list_init(&hdr->childs, NULL);
    hdr->layer      = 0;
    hdr->guard_code = TREE_GURAD_CODE;
    hdr->is_init    = 1;
    return TREE_OK;
}

static int _uninit_tree_hdr(void* node)
{
    CHECK_IF(node == NULL, return TREE_FAIL, "node is null");

    struct tree_hdr* hdr = (struct tree_hdr*)node;
    CHECK_IF(hdr->is_init != 1, return TREE_FAIL, "node is not init yet");
    CHECK_IF(hdr->guard_code != TREE_GURAD_CODE, return TREE_FAIL, "guard code is error");

    list_clean(&hdr->childs);
    hdr->tree       = NULL;
    hdr->parent     = NULL;
    hdr->guard_code = 0;
    hdr->is_init    = 0;
    hdr->layer      = 0;
    return TREE_OK;
}

static int _is_layer_bigger_than_me(void* target, void* me)
{
    CHECK_IF(target == NULL, return 0, "target is null");
    CHECK_IF(me == NULL, return 0, "me is null");

    struct tree_hdr* node = (struct tree_hdr*)target;
    struct tree_hdr* mynode = (struct tree_hdr*)me;
    return (node->layer > mynode->layer) ? 1 : 0;
}

int tree_init(struct tree* tree, void* root, void (*cleanfn)(void*))
{
    CHECK_IF(tree == NULL, return TREE_FAIL, "tree is null");
    CHECK_IF(root == NULL, return TREE_FAIL, "root is null");

    memset(tree, 0, sizeof(struct tree));

    int chk = list_init(&tree->nodes, cleanfn);
    CHECK_IF(chk != LIST_OK, return TREE_FAIL, "list_init failed");

    chk = _init_tree_hdr(root, NULL, tree);
    CHECK_IF(chk != TREE_OK, return TREE_FAIL, "_init_tree_hdr failed");

    chk = list_append(&tree->nodes, root);
    CHECK_IF(chk != LIST_OK, return TREE_FAIL, "list_append root failed");

    tree->is_init  = 1;
    return TREE_OK;
}

int tree_clean(struct tree* tree)
{
    CHECK_IF(tree == NULL, return TREE_FAIL, "tree is null");
    CHECK_IF(tree->is_init != 1, return TREE_FAIL, "tree is not init yet");

    void* node;
    void* obj;
    LIST_FOREACH(&tree->nodes, obj, node)
    {
        _uninit_tree_hdr(node);
    }
    list_clean(&tree->nodes);
    tree->is_init = 0;
    return TREE_OK;
}

int tree_add(void* parent, void* child)
{
    CHECK_IF(parent == NULL, return TREE_FAIL, "parent is null");
    CHECK_IF(child == NULL, return TREE_FAIL, "child is null");
    CHECK_IF(parent == child, return TREE_FAIL, "parent = child");
    CHECK_IF(!_is_node_unused(child), return TREE_FAIL, "child is already in some tree");

    struct tree_hdr* parent_hdr = (struct tree_hdr*)parent;
    struct tree* tree           = parent_hdr->tree;

    int chk = _init_tree_hdr(child, parent, tree);
    CHECK_IF(chk != TREE_OK, return TREE_FAIL, "_init_tree_hdr failed");

    struct tree_hdr* child_hdr  = (struct tree_hdr*)child;
    child_hdr->layer = parent_hdr->layer + 1;

    int list_ret = list_append(&parent_hdr->childs, child);
    CHECK_IF(list_ret != LIST_OK, goto _ERROR, "list_append to parent failed");

    struct list_node* target_node = list_find_node(&tree->nodes, _is_layer_bigger_than_me, child);
    if (target_node == NULL)
    {
        target_node = list_tail_node(&tree->nodes);
        if (target_node == NULL)
        {
            list_ret = list_append(&tree->nodes, child);
            CHECK_IF(list_ret != LIST_OK, goto _ERROR, "list_append to tree failed");
        }
        else
        {
            list_ret = list_append_after_node(&tree->nodes, target_node, child);
            CHECK_IF(list_ret != LIST_OK, goto _ERROR, "list_append_after_node failed");
        }
    }
    else
    {
        list_ret = list_insert_before_node(&tree->nodes, target_node, child);
        CHECK_IF(list_ret != LIST_OK, goto _ERROR, "list_insert_before_node failed");
    }
    return TREE_OK;

_ERROR:
    list_remove(&parent_hdr->childs, child);
    list_remove(&tree->nodes, child);
    _uninit_tree_hdr(child);
    return TREE_FAIL;
}

int tree_remove(void* parent, void* child)
{
    CHECK_IF(parent == NULL, return TREE_FAIL, "parent is null");
    CHECK_IF(child == NULL, return TREE_FAIL, "child is null");
    CHECK_IF(parent == child, return TREE_FAIL, "parent = child");
    CHECK_IF(_is_node_unused(child), return TREE_FAIL, "child is not in any tree");
    CHECK_IF(!tree_is_leaf(child), return TREE_FAIL, "child is not a leaf");

    struct tree_hdr* parent_hdr = (struct tree_hdr*)parent;
    struct tree* tree           = parent_hdr->tree;

    int list_ret;
    list_ret = list_remove(&parent_hdr->childs, child);
    CHECK_IF(list_ret != LIST_OK, return TREE_FAIL, "child is not born by parent");

    list_ret = list_remove(&tree->nodes, child);
    CHECK_IF(list_ret != LIST_OK, return TREE_FAIL, "child is not in this tree");

    return _uninit_tree_hdr(child);
}

int tree_is_leaf(void* node)
{
    CHECK_IF(node == NULL, return 0, "node is null");
    CHECK_IF(_is_node_unused(node), return 0, "node is not added to any tree");

    struct tree_hdr* hdr = (struct tree_hdr*)node;

    if (hdr->parent == NULL) return 0; // root
    if (list_num(&hdr->childs) > 0) return 0; // node has children

    return 1;
}

int tree_is_ancestor(void* descendant, void* ancestor)
{
    CHECK_IF(descendant == NULL, return 0, "descendant is null");
    CHECK_IF(ancestor == NULL, return 0, "ancestor is null");
    CHECK_IF(descendant == ancestor, return 0, "descendant = ancestor");

    struct tree_hdr* des = (struct tree_hdr*)descendant;
    struct tree_hdr* anc = (struct tree_hdr*)ancestor;
    struct tree_hdr* node;
    for (node = des->parent; node; node = node->parent)
    {
        if (node == anc) return 1;
    }
    return 0;
}

void* tree_common_ancestor(void* node1, void* node2)
{
    CHECK_IF(node1 == NULL, return NULL, "node1 is null");
    CHECK_IF(node2 == NULL, return NULL, "node2 is null");
    CHECK_IF(node1 == node2, return NULL, "node1 = node2");

    if (tree_is_ancestor(node1, node2))
    {
        return node2;
    }
    else if (tree_is_ancestor(node2, node1))
    {
        return node1;
    }
    else
    {
        struct tree_hdr* hdr1 = (struct tree_hdr*)node1;
        struct tree_hdr* ancestor;
        for (ancestor = hdr1->parent; ancestor; ancestor = ancestor->parent)
        {
            if (tree_is_ancestor(node2, ancestor)) return ancestor;
        }
    }

    return NULL;
}

int tree_route(void* src, void* dst, struct list* route)
{
    CHECK_IF(src == NULL, return TREE_FAIL, "src is null");
    CHECK_IF(dst == NULL, return TREE_FAIL, "dst is null");
    CHECK_IF(src == dst, return TREE_FAIL, "src = dst");
    CHECK_IF(route == NULL, return TREE_FAIL, "route is null");
    CHECK_IF(_is_node_unused(src), return TREE_FAIL, "src is not add to tree.");
    CHECK_IF(_is_node_unused(dst), return TREE_FAIL, "dst is not add to tree.");

    struct tree_hdr* srchdr = (struct tree_hdr*)src;
    struct tree_hdr* dsthdr = (struct tree_hdr*)dst;
    CHECK_IF(srchdr->tree != dsthdr->tree, return TREE_FAIL, "src and dst are in diff tree.");

    list_clean(route);

    void* cmn_ancestor = tree_common_ancestor(src, dst);
    CHECK_IF(cmn_ancestor == NULL, return TREE_FAIL, "no common ancestor");

    int chk;
    struct tree_hdr* node;
    for (node = src; node != cmn_ancestor; node = node->parent)
    {
        chk = list_append(route, node);
        CHECK_IF(chk != LIST_OK, goto _ERROR, "record src to ancestor failed");
    }

    chk = list_append(route, cmn_ancestor);
    CHECK_IF(chk != LIST_OK, goto _ERROR, "record ancestor failed");

    for (node = dst; node != cmn_ancestor; node = node->parent)
    {
        chk = list_append_after(route, cmn_ancestor, node);
        CHECK_IF(chk != LIST_OK, goto _ERROR, "record ancestor to dst failed");
    }

    return TREE_OK;

_ERROR:
    list_clean(route);
    return TREE_FAIL;
}

int tree_dfs(struct tree* tree, void (*execfn)(void* node, int layer))
{
    CHECK_IF(tree == NULL, return TREE_FAIL, "tree is null");
    CHECK_IF(execfn == NULL, return TREE_FAIL, "execfn is null");
    CHECK_IF(tree->is_init != 1, return TREE_FAIL, "tree is not init yet");

    void* root = list_head(&tree->nodes);
    CHECK_IF(root == NULL, return TREE_FAIL, "tree is empty");

    return _dfs(root, execfn);
}

int tree_bfs(struct tree* tree, void (*execfn)(void* node, int layer))
{
    CHECK_IF(tree == NULL, return TREE_FAIL, "tree is null");
    CHECK_IF(execfn == NULL, return TREE_FAIL, "execfn is null");
    CHECK_IF(tree->is_init != 1, return TREE_FAIL, "tree is not init yet");

    void* node;
    void* obj;
    LIST_FOREACH(&tree->nodes, obj, node)
    {
        execfn(node, ((struct tree_hdr*)node)->layer);
    }
    return TREE_OK;
}

struct list* tree_childs(void* node)
{
    CHECK_IF(node == NULL, return NULL, "node is null");
    CHECK_IF(_is_node_unused(node), return NULL, "node is not added to any tree");

    struct tree_hdr* hdr = (struct tree_hdr*)node;
    return &(hdr->childs);
}

void* tree_parent(void* node)
{
    CHECK_IF(node == NULL, return NULL, "node is null");
    CHECK_IF(_is_node_unused(node), return NULL, "node is not added to any tree");

    struct tree_hdr* hdr = (struct tree_hdr*)node;
    return hdr->parent;
}
