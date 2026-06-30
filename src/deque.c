#include "nova.h"
#include <stdlib.h>

/* a fixed-capacity double-ended queue of ints over a circular buffer. */

int nova_deque_init(nova_deque *d, int cap)
{
    if (cap < 2) cap = 2;
    d->buf = (int*)malloc(sizeof(int) * cap);
    if (!d->buf) return -1;
    d->cap = cap;
    d->head = d->tail = d->count = 0;
    return 0;
}

int nova_deque_push_back(nova_deque *d, int v)
{
    if (!d->buf || d->count >= d->cap) return -1;
    d->buf[d->tail] = v;
    d->tail = (d->tail + 1) % d->cap;
    d->count++;
    return 0;
}

int nova_deque_push_front(nova_deque *d, int v)
{
    if (!d->buf || d->count >= d->cap) return -1;
    d->head = (d->head - 1 + d->cap) % d->cap;
    d->buf[d->head] = v;
    d->count++;
    return 0;
}

int nova_deque_pop_front(nova_deque *d, int *out)
{
    if (!d->buf || d->count <= 0) return -1;
    if (out) *out = d->buf[d->head];
    d->head = (d->head + 1) % d->cap;
    d->count--;
    return 0;
}

int nova_deque_pop_back(nova_deque *d, int *out)
{
    if (!d->buf || d->count <= 0) return -1;
    d->tail = (d->tail - 1 + d->cap) % d->cap;
    if (out) *out = d->buf[d->tail];
    d->count--;
    return 0;
}

void nova_deque_free(nova_deque *d)
{
    free(d->buf);
    d->buf = NULL;
    d->cap = d->head = d->tail = d->count = 0;
}
