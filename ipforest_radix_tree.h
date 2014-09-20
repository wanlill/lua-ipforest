#ifndef IPFOREST_RADIX_TREE
#define IPFOREST_RADIX_TREE

#include "ipforest_types.h"

typedef struct ipforest_radix_tree_node_s {
    struct ipforest_radix_tree_node_s *p;    /* parent */
    struct ipforest_radix_tree_node_s *l;   /* left child */
    struct ipforest_radix_tree_node_s *r;   /* right child */
    ipforest_list_entry_t entry;   /* next if in free list or used list */
} ipforest_radix_tree_node_t;

typedef struct ipforest_radix_tree_s {
    ipforest_radix_tree_node_t root;
    ipforest_list_entry_t used;
    ipforest_list_entry_t free;
} ipforest_radix_tree_t;

ipforest_radix_tree_t * ipforest_radix_tree_alloc();
void ipforest_radix_tree_free(ipforest_radix_tree_t *tree);
void ipforest_radix_tree_compact(ipforest_radix_tree_t *tree);
IPFOREST_BOOLEAN ipforest_radix_tree_insert(ipforest_radix_tree_t *tree, uint32_t addr, uint32_t mask);
IPFOREST_BOOLEAN ipforest_radix_tree_lookup(ipforest_radix_tree_t *tree, uint32_t addr, uint32_t mask);

#endif
