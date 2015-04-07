#include <string.h>

#include "basic.h"
#include "tree.h"

typedef struct tTaco
{
    tTreeHdr hdr;
    char name[20];

} tTaco;

static char _tabs[100];

static void _printTaco(void* node, int layer)
{
    tTaco *taco = (tTaco*)node;
    dprint("%.*s[%s]", layer*4, _tabs, taco->name);
    return;
}

int main(int argc, char const *argv[])
{
    tTree tree = {};

    memset(_tabs, ' ', 100);

    tTaco root = {.name = "root"};
    tree_init(&tree, &root, NULL, _printTaco);

    // root
    // +-- node1
    //     +-- node11
    //         +-- node111
    //         +-- node112
    //     +-- node12
    // +-- node2
    //     +-- node21
    // +-- node3

    tTaco node1 = {.name = "node1"};
    tTaco node2 = {.name = "node2"};
    tTaco node3 = {.name = "node3"};

    tree_add(&root, &node1);
    tree_add(&root, &node2);
    tree_add(&root, &node3);

    tTaco node11 = {.name = "node11"};
    tTaco node12 = {.name = "node12"};
    tTaco node21 = {.name = "node21"};

    tree_add(&node1, &node11);
    tree_add(&node1, &node12);
    tree_add(&node2, &node21);

    tTaco node111 = {.name = "node111"};
    tTaco node112 = {.name = "node112"};

    tree_add(&node11, &node111);
    tree_add(&node11, &node112);

    tree_add(&node1, &node3); // fail , print error message

    tree_print(&tree);

    tTaco* ancestor = tree_commonAncestor(&node21, &node111);
    dprint("common ancestor of %s and %s : %s", node21.name, node111.name, ancestor->name);

    ancestor = tree_commonAncestor(&node12, &node111);
    dprint("common ancestor of %s and %s : %s", node12.name, node111.name, ancestor->name);

    tList route = {};
    list_init(&route, NULL);

    tree_route(&node111, &node3, &route);
    dprint("route from %s to %s:", node111.name, node3.name);
    tTaco* tmp;
    tListObj* obj;
    LIST_FOREACH(&route, obj, tmp)
    {
        dprint("%s", tmp->name);
    }
    list_clean(&route);
    dprint("");

    tree_route(&node11, &node12, &route);
    dprint("route from %s to %s:", node11.name, node12.name);
    LIST_FOREACH(&route, obj, tmp)
    {
        dprint("%s", tmp->name);
    }
    list_clean(&route);
    dprint("");

    tree_route(&node112, &node1, &route);
    dprint("route from %s to %s:", node112.name, node1.name);
    LIST_FOREACH(&route, obj, tmp)
    {
        dprint("%s", tmp->name);
    }
    list_clean(&route);
    dprint("");

    tree_route(&node1, &node111, &route);
    dprint("route from %s to %s:", node1.name, node111.name);
    LIST_FOREACH(&route, obj, tmp)
    {
        dprint("%s", tmp->name);
    }
    list_clean(&route);
    dprint("");

    tree_remove(&node11, &node111);
    tree_remove(&node21, &node112); // fail, print error message
    tree_remove(&node11, &node112);

    tree_remove(&node1, &node11);
    tree_remove(&node1, &node12);

    tree_remove(&node2, &node21);

    tree_remove(&root, &node1);
    tree_remove(&root, &node2);
    tree_remove(&root, &node3);

    tree_print(&tree);

    tree_clean(&tree);
    return 0;
}
