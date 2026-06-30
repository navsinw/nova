#include "nova.h"
#include <stdlib.h>

/* binary min-heap keyed by int priority, carrying an int payload. used as the
   open set for graph search when a linear scan would be too slow. */

int nova_heap_init(nova_heap *h, int cap)
{
    if (cap < 4) cap = 4;
    h->key = (int*)malloc(sizeof(int) * cap);
    h->val = (int*)malloc(sizeof(int) * cap);
    if (!h->key || !h->val) { free(h->key); free(h->val); h->key = h->val = NULL; return -1; }
    h->cap = cap;
    h->n = 0;
    return 0;
}

static void swap_at(nova_heap *h, int a, int b)
{
    int tk = h->key[a]; h->key[a] = h->key[b]; h->key[b] = tk;
    int tv = h->val[a]; h->val[a] = h->val[b]; h->val[b] = tv;
}

static int grow(nova_heap *h)
{
    int nc = h->cap * 2;
    int *nk = (int*)realloc(h->key, sizeof(int) * nc);
    int *nv = (int*)realloc(h->val, sizeof(int) * nc);
    if (!nk || !nv) { free(nk); free(nv); return -1; }
    h->key = nk; h->val = nv; h->cap = nc;
    return 0;
}

int nova_heap_push(nova_heap *h, int key, int val)
{
    if (h->n >= h->cap && grow(h) != 0) return -1;
    int i = h->n++;
    h->key[i] = key; h->val[i] = val;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->key[parent] <= h->key[i]) break;
        swap_at(h, parent, i);
        i = parent;
    }
    return 0;
}

int nova_heap_pop(nova_heap *h, int *key, int *val)
{
    if (h->n <= 0) return -1;
    if (key) *key = h->key[0];
    if (val) *val = h->val[0];
    h->n--;
    h->key[0] = h->key[h->n];
    h->val[0] = h->val[h->n];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < h->n && h->key[l] < h->key[m]) m = l;
        if (r < h->n && h->key[r] < h->key[m]) m = r;
        if (m == i) break;
        swap_at(h, i, m);
        i = m;
    }
    return 0;
}

int nova_heap_peek(const nova_heap *h, int *key, int *val)
{
    if (h->n <= 0) return -1;
    if (key) *key = h->key[0];
    if (val) *val = h->val[0];
    return 0;
}

void nova_heap_free(nova_heap *h)
{
    free(h->key); free(h->val);
    h->key = h->val = NULL;
    h->n = h->cap = 0;
}
