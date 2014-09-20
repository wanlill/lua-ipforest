#include <stdlib.h>
#include <string.h>
#include "ipforest_types.h"
#include "ipforest_radix_tree.h"

inline static ipforest_radix_tree_node_t *
_get_node(ipforest_radix_tree_t *tree)
{
    ipforest_list_entry_t *node;
    ipforest_radix_tree_node_t *ret;

    if (!_list_empty(&tree->free)) {
        node = tree->free.next;
        _list_unlink(node);
        ret = CONTAINER_OF(node, entry, ipforest_radix_tree_node_t);
    } else {
        ret = malloc(sizeof(ipforest_radix_tree_node_t));
        if (ret) {
            memset(ret, 0, sizeof(ipforest_radix_tree_node_t));
        }
    }
    
    _list_insert_after(&tree->used, &ret->entry);

    return ret;
}

inline static void
_free_node(ipforest_radix_tree_t *tree, ipforest_radix_tree_node_t *node)
{
    _list_unlink(&node->entry);
    memset(node, 0, sizeof(ipforest_radix_tree_node_t));
    _list_insert_after(&tree->free, &node->entry);
}

inline static IPFOREST_BOOLEAN
_is_leaf(ipforest_radix_tree_node_t *node)
{
    return (node->l == node->r) && (node->l == node);
}

inline static void
_make_leaf(ipforest_radix_tree_node_t *node)
{
    node->l = node->r = node;
}

inline static ipforest_radix_tree_node_t *
_get_sibling(ipforest_radix_tree_node_t *node)
{
    if (node->p) {
        return node->p->l == node ?
            node->p->r : node->p->l;
    }
    return NULL;
}

/*
 * prune tree down, assume that node is a leaf-tobe
 */
inline static void
_prune_tree_down(ipforest_radix_tree_t *tree, ipforest_radix_tree_node_t *p)
{
    ipforest_list_entry_t head, *next;
    ipforest_radix_tree_node_t *node;

    _list_init(&head);
    node = p;

    if (node->l) {
        /* unlink from used list */
        _list_unlink(&node->l->entry);
        _list_insert_before(&head, &node->l->entry);
    }

    if (node->r) {
        /* unlink from used list */
        _list_unlink(&node->r->entry);
        _list_insert_before(&head, &node->r->entry);
    }

    while (!_list_empty(&head)) {
        next = head.next;
        node = CONTAINER_OF(next, entry, ipforest_radix_tree_node_t);

        if (node->l) {
            /* unlink from used list */
            _list_unlink(&node->l->entry);
            _list_insert_before(&head, &node->l->entry);
        }

        if (node->r) {
            /* unlink from used list */
            _list_unlink(&node->r->entry);
            _list_insert_before(&head, &node->r->entry);
        }

        _free_node(tree, node);
    }
}

/*
 * prune tree up, assume that node is a leaf when called
 */
inline static void
_prune_tree_up(ipforest_radix_tree_t *tree, ipforest_radix_tree_node_t *node)
{
    ipforest_radix_tree_node_t *cur, *sibling;
    cur = node;

    do {
        sibling = _get_sibling(cur);
        if (!sibling) {
            /* maybe root and maybe not sibling */
            _make_leaf(cur);
            return;
        } else {
            if (_is_leaf(sibling)) {
                /* go top, parent is also leaf, also delete the node and the sibling */
                cur = cur->p;
                _free_node(tree, _get_sibling(sibling));
                _free_node(tree, sibling);
            } else {
                _make_leaf(cur);
                return;
            }
        }
    } while (IPFOREST_TRUE);
}

ipforest_radix_tree_t *
ipforest_radix_tree_alloc()
{
    ipforest_radix_tree_t *ret;
    ret = malloc(sizeof(ipforest_radix_tree_t));
    if (ret) {
        memset(ret, 0, sizeof(ipforest_radix_tree_t));
        _list_init(&ret->used);
        _list_init(&ret->free);
    }
    return ret;
}

void
ipforest_radix_tree_free(ipforest_radix_tree_t *tree)
{
    ipforest_list_entry_t *p, *safe;

    LIST_FOREACH(&tree->used, p, safe) {
        _list_unlink(p);
        free(CONTAINER_OF(p, entry, ipforest_radix_tree_node_t));
    }

    LIST_FOREACH(&tree->free, p, safe) {
        _list_unlink(p);
        free(CONTAINER_OF(p, entry, ipforest_radix_tree_node_t));
    }

    free(tree);
}

void
ipforest_radix_tree_compact(ipforest_radix_tree_t *tree)
{
    ipforest_list_entry_t *p, *safe;

    LIST_FOREACH(&tree->free, p, safe) {
        _list_unlink(p);
        free(CONTAINER_OF(p, entry, ipforest_radix_tree_node_t));
    }
}

IPFOREST_BOOLEAN
ipforest_radix_tree_insert(ipforest_radix_tree_t *tree, uint32_t addr, uint32_t mask)
{
    uint32_t m;
    ipforest_radix_tree_node_t *cur, **pnext, *node;

    cur = &tree->root;
    m = 1 << 31;
    node = NULL;

    if (_is_leaf(cur)) {
        return IPFOREST_TRUE;
    }

    while (m & mask) {
        if (m & addr) {
            pnext = &cur->r;
        } else {
            pnext = &cur->l;
        }

        if (!(*pnext)) {
            /* no one is here */
            node = _get_node(tree);
            if (!node) {
                return IPFOREST_FALSE;
            }
            *pnext = node;
            node->p = cur;
            cur = *pnext;
        } else {
            if (_is_leaf(*pnext)) {
                return IPFOREST_TRUE;
            } else {
                cur = *pnext;
            }
        }

        m = m >> 1;
    }

    if (!node) {
        /* we've exhaust the mask and still meeting old node */
        _prune_tree_down(tree, cur);
    }

    /* we've created new node or extended old node */
    if (_get_sibling(cur) && _is_leaf(_get_sibling(cur))) {
        /* prune up*/
        _prune_tree_up(tree, cur);
    } else {
        /* mark the new node as leaf */
        _make_leaf(cur);
    }

    return IPFOREST_TRUE;
}

IPFOREST_BOOLEAN
ipforest_radix_tree_lookup(ipforest_radix_tree_t *tree, uint32_t addr, uint32_t mask)
{
    uint32_t m;
    ipforest_radix_tree_node_t *cur;
    
    m = 1 << 31;
    cur = &tree->root;

    while (m & mask) {
        if (_is_leaf(cur)) {
            return IPFOREST_TRUE;
        }
        if (addr & m) {
            cur = cur->r;
        } else {
            cur = cur->l;
        }
        
        if (!cur) {
            return IPFOREST_FALSE;
        }

        m = m >> 1;
    }

    if (_is_leaf(cur)) {
        return IPFOREST_TRUE;
    } else {
        return IPFOREST_FALSE;
    }
}
