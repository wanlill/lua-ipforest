#ifndef IPFOREST_TYPES
#define IPFOREST_TYPES

#include <stdint.h>
#include <assert.h>

typedef struct ipforest_ipaddr_s {
    uint32_t addr;
    uint32_t mask;
} ipforest_ipaddr_t;

typedef enum {
    IPFOREST_FALSE,
    IPFOREST_TRUE
} IPFOREST_BOOLEAN;

#define IPFOREST_IPSTR_MAX_LEN 15
#define IPFOREST_IPSTR_BUF_LEN 32

/* [i, 31] is set to 1 */
#define IPFOREST_MASK_UP(i) ((uint32_t) \
                             (0xffffffff ^ ((1 << (i)) - 1)))

/* [0, i] is set to 1 */
#define IPFOREST_MASK_DOWN(i) ((uint32_t) \
                               ((1 << ((i) + 1)) - 1))

typedef struct ipforest_list_entry_s {
    struct ipforest_list_entry_s *prev;
    struct ipforest_list_entry_s *next;
} ipforest_list_entry_t;

inline static void
_list_init(ipforest_list_entry_t *head)
{
    head->prev = head;
    head->next = head;
}

inline static IPFOREST_BOOLEAN
_list_empty(ipforest_list_entry_t *node)
{
    return node->next == node && node->prev == node;
}

inline static void
_list_insert_after(ipforest_list_entry_t *old, ipforest_list_entry_t *new)
{
    ipforest_list_entry_t *next;

    next = old->next;
    
    old->next = new;
    new->prev = old;
    
    new->next = next;
    next->prev = new;
}

inline static void
_list_insert_before(ipforest_list_entry_t *old, ipforest_list_entry_t *new)
{
    ipforest_list_entry_t *prev;

    prev = old->prev;

    old->prev = new;
    new->next = old;

    new->prev = prev;
    prev->next = new;
}

inline static void
_list_unlink(ipforest_list_entry_t *node)
{
    /* should not delete a empty list */
    assert(!_list_empty(node));

    node->prev->next = node->next;
    node->next->prev = node->prev;
}

#define CONTAINER_OF(p, field, container) \
    ((container *)((size_t)p - (size_t)(&(((container *)0)->field))))

#define LIST_FOREACH(head, p, safep) \
    for (p = (head)->next, safep = p->next; p != head; p = safep, safep = p->next)

#endif
