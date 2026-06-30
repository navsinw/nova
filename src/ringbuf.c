#include "nova.h"
#include <stdlib.h>

/* byte ring buffer used for streaming input queues. */

int nova_ring_init(nova_ring *r, int cap)
{
    if (cap < 2) cap = 2;
    r->buf = (uint8_t*)malloc((size_t)cap);
    if (!r->buf) return -1;
    r->cap = cap;
    r->head = r->tail = r->count = 0;
    return 0;
}

int nova_ring_push(nova_ring *r, uint8_t v)
{
    if (!r->buf || r->count >= r->cap) return -1;
    r->buf[r->tail] = v;
    r->tail = (r->tail + 1) % r->cap;
    r->count++;
    return 0;
}

int nova_ring_pop(nova_ring *r, uint8_t *out)
{
    if (!r->buf || r->count <= 0) return -1;
    uint8_t v = r->buf[r->head];
    r->head = (r->head + 1) % r->cap;
    r->count--;
    if (out) *out = v;
    return 0;
}

int nova_ring_count(const nova_ring *r)
{
    return r->count;
}

void nova_ring_free(nova_ring *r)
{
    free(r->buf);
    r->buf = NULL;
    r->cap = r->head = r->tail = r->count = 0;
}
