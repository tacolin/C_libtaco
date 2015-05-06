#include <string.h>

#include "basic.h"
#include "tree.h"

struct taco
{
    struct tree_hdr hdr;
    char name[20];
};

static char _tabs[100];

static void _print_taco(void* node, int layer)
{
    struct taco *taco = (struct taco*)node;
    dprint("%.*s[%s]", layer*4, _tabs, taco->name);
    return;
}

int main(int argc, char const *argv[])
{
    struct tree tree = {};

    memset(_tabs, ' ', 100);

    struct taco root = {.name = "root"};
    tree_init(&tree, &root, NULL);

    // root
    // +-- node1
    //     +-- node11
    //         +-- node111
    //         +-- node112
    //     +-- node12
    // +-- node2
    //     +-- node21
    // +-- node3

    struct taco node1 = {.name = "node1"};
    struct taco node11 = {.name = "node11"};
    struct taco node12 = {.name = "node12"};

    tree_add(&root, &node1);
    tree_add(&node1, &node11);
    tree_add(&node1, &node12);

    struct taco node2 = {.name = "node2"};
    struct taco node21 = {.name = "node21"};

    tree_add(&root, &node2);
    tree_add(&node2, &node21);

    struct taco node3 = {.name = "node3"};
    struct taco node111 = {.name = "node111"};
    struct taco node112 = {.name = "node112"};

    tree_add(&root, &node3);
    tree_add(&node11, &node111);
    tree_add(&node11, &node112);

    tree_add(&node1, &node3); // fail , print error message

    tree_dfs(&tree, _print_taco);

    dprint("");

    tree_bfs(&tree, _print_taco);

    struct taco* ancestor = tree_common_ancestor(&node21, &node111);
    dprint("common ancestor of %s and %s : %s", node21.name, node111.name, ancestor->name);

    ancestor = tree_common_ancestor(&node12, &node111);
    dprint("common ancestor of %s and %s : %s", node12.name, node111.name, ancestor->name);

    struct list route = {};
    list_init(&route, NULL);

    tree_route(&node111, &node3, &route);
    dprint("route from %s to %s:", node111.name, node3.name);
    struct taco* tmp;
    void* obj;
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

    tree_dfs(&tree, _print_taco);

    tree_clean(&tree);
    return 0;
}
