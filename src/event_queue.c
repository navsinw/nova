#include "nova.h"
#include <stdlib.h>

/* a bounded FIFO of game events (type + two payload ints). */

int nova_evq_init(nova_eventq *q, int cap)
{
    if (cap < 2) cap = 2;
    q->ev = (nova_event*)malloc((size_t)cap * sizeof(nova_event));
    if (!q->ev) return -1;
    q->cap = cap;
    q->head = q->tail = q->count = 0;
    return 0;
}

int nova_evq_push(nova_eventq *q, int type, int a, int b)
{
    if (!q->ev || q->count >= q->cap) return -1;
    q->ev[q->tail].type = type;
    q->ev[q->tail].a = a;
    q->ev[q->tail].b = b;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    return 0;
}

int nova_evq_pop(nova_eventq *q, nova_event *out)
{
    if (!q->ev || q->count <= 0) return -1;
    if (out) *out = q->ev[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    return 0;
}

void nova_evq_free(nova_eventq *q)
{
    free(q->ev);
    q->ev = NULL;
    q->cap = q->head = q->tail = q->count = 0;
}
