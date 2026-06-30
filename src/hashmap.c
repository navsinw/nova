#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* open-addressing string->int map with linear probing and tombstones. */

int nova_hm_init(nova_hashmap *h, int cap)
{
    if (cap < 8) cap = 8;
    /* round up to power of two */
    int c = 8;
    while (c < cap) c <<= 1;
    h->slots = (nova_hm_slot*)calloc(c, sizeof(nova_hm_slot));
    if (!h->slots) return -1;
    h->cap = c;
    h->n = 0;
    return 0;
}

static uint32_t hkey(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
}

static int grow(nova_hashmap *h)
{
    nova_hashmap nh;
    if (nova_hm_init(&nh, h->cap * 2) != 0) return -1;
    for (int i = 0; i < h->cap; i++)
        if (h->slots[i].used == 1)
            nova_hm_put(&nh, h->slots[i].key, h->slots[i].val);
    for (int i = 0; i < h->cap; i++)
        if (h->slots[i].used == 1) free(h->slots[i].key);
    free(h->slots);
    *h = nh;
    return 0;
}

int nova_hm_put(nova_hashmap *h, const char *key, int val)
{
    if (!h->slots) return -1;
    if ((h->n + 1) * 4 >= h->cap * 3) { if (grow(h) != 0) return -1; }
    uint32_t mask = (uint32_t)h->cap - 1;
    uint32_t i = hkey(key) & mask;
    int first_free = -1;
    for (int probe = 0; probe < h->cap; probe++) {
        nova_hm_slot *s = &h->slots[i];
        if (s->used == 0) {
            int slot = first_free >= 0 ? first_free : (int)i;
            size_t kl = strlen(key);
            h->slots[slot].key = (char*)malloc(kl + 1);
            if (!h->slots[slot].key) return -1;
            memcpy(h->slots[slot].key, key, kl + 1);
            h->slots[slot].val = val;
            h->slots[slot].used = 1;
            h->n++;
            return 0;
        }
        if (s->used == 2) { if (first_free < 0) first_free = (int)i; }
        else if (strcmp(s->key, key) == 0) { s->val = val; return 0; }
        i = (i + 1) & mask;
    }
    return -1;
}

int nova_hm_get(nova_hashmap *h, const char *key, int *out)
{
    if (!h->slots) return -1;
    uint32_t mask = (uint32_t)h->cap - 1;
    uint32_t i = hkey(key) & mask;
    for (int probe = 0; probe < h->cap; probe++) {
        nova_hm_slot *s = &h->slots[i];
        if (s->used == 0) return -1;
        if (s->used == 1 && strcmp(s->key, key) == 0) { if (out) *out = s->val; return 0; }
        i = (i + 1) & mask;
    }
    return -1;
}

int nova_hm_del(nova_hashmap *h, const char *key)
{
    if (!h->slots) return -1;
    uint32_t mask = (uint32_t)h->cap - 1;
    uint32_t i = hkey(key) & mask;
    for (int probe = 0; probe < h->cap; probe++) {
        nova_hm_slot *s = &h->slots[i];
        if (s->used == 0) return -1;
        if (s->used == 1 && strcmp(s->key, key) == 0) {
            free(s->key); s->key = NULL; s->used = 2; h->n--;
            return 0;
        }
        i = (i + 1) & mask;
    }
    return -1;
}

void nova_hm_free(nova_hashmap *h)
{
    if (!h->slots) return;
    for (int i = 0; i < h->cap; i++)
        if (h->slots[i].used == 1) free(h->slots[i].key);
    free(h->slots);
    h->slots = NULL; h->cap = h->n = 0;
}
