#include "nova.h"
#include <stdlib.h>

/* doubly linked list of ints. */

void nova_list_init(nova_list *l)
{
    l->head = l->tail = NULL;
    l->count = 0;
}

int nova_list_push_back(nova_list *l, int v)
{
    nova_lnode *n = (nova_lnode*)malloc(sizeof(nova_lnode));
    if (!n) return -1;
    n->val = v; n->next = NULL; n->prev = l->tail;
    if (l->tail) l->tail->next = n; else l->head = n;
    l->tail = n;
    l->count++;
    return 0;
}

int nova_list_push_front(nova_list *l, int v)
{
    nova_lnode *n = (nova_lnode*)malloc(sizeof(nova_lnode));
    if (!n) return -1;
    n->val = v; n->prev = NULL; n->next = l->head;
    if (l->head) l->head->prev = n; else l->tail = n;
    l->head = n;
    l->count++;
    return 0;
}

int nova_list_pop_front(nova_list *l, int *out)
{
    if (!l->head) return -1;
    nova_lnode *n = l->head;
    if (out) *out = n->val;
    l->head = n->next;
    if (l->head) l->head->prev = NULL; else l->tail = NULL;
    free(n);
    l->count--;
    return 0;
}

int nova_list_remove(nova_list *l, int v)
{
    int removed = 0;
    nova_lnode *n = l->head;
    while (n) {
        nova_lnode *nx = n->next;
        if (n->val == v) {
            if (n->prev) n->prev->next = n->next; else l->head = n->next;
            if (n->next) n->next->prev = n->prev; else l->tail = n->prev;
            free(n);
            l->count--;
            removed++;
        }
        n = nx;
    }
    return removed;
}

void nova_list_free(nova_list *l)
{
    nova_lnode *n = l->head;
    while (n) { nova_lnode *nx = n->next; free(n); n = nx; }
    l->head = l->tail = NULL;
    l->count = 0;
}
