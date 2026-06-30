#include "nova.h"
#include <stdlib.h>

/* dice rolls, weighted selection, and a shuffle-bag (each item drawn once per
   refill, then reshuffled) for fair-feeling randomness. */

int nova_roll(nova_rng *r, int sides, int count)
{
    if (sides < 1) sides = 1;
    if (count < 1) count = 1;
    if (count > 100) count = 100;
    int total = 0;
    for (int i = 0; i < count; i++)
        total += 1 + (int)(nova_rng_next(r) % (uint32_t)sides);
    return total;
}

int nova_weighted_choice(nova_rng *r, const int *weights, int n)
{
    if (!weights || n <= 0) return -1;
    long sum = 0;
    for (int i = 0; i < n; i++) if (weights[i] > 0) sum += weights[i];
    if (sum <= 0) return -1;
    long pick = (long)(nova_rng_next(r) % (uint32_t)sum);
    for (int i = 0; i < n; i++) {
        if (weights[i] <= 0) continue;
        pick -= weights[i];
        if (pick < 0) return i;
    }
    return n - 1;
}

int nova_bag_init(nova_bag *b, int cap, uint32_t seed)
{
    if (cap < 1) cap = 1;
    b->items = (int*)malloc(sizeof(int) * cap);
    if (!b->items) return -1;
    b->cap = cap;
    b->n = 0;
    b->pos = 0;
    nova_rng_seed(&b->rng, seed);
    return 0;
}

int nova_bag_add(nova_bag *b, int item)
{
    if (!b->items || b->n >= b->cap) return -1;
    b->items[b->n++] = item;
    return 0;
}

int nova_bag_draw(nova_bag *b)
{
    if (!b->items || b->n <= 0) return -1;
    if (b->pos >= b->n) {
        nova_rng_shuffle(&b->rng, b->items, b->n);
        b->pos = 0;
    }
    return b->items[b->pos++];
}

void nova_bag_free(nova_bag *b)
{
    free(b->items);
    b->items = NULL;
    b->n = b->cap = b->pos = 0;
}
