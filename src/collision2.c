#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* AABB tests and a uniform-grid spatial hash for broadphase queries. */

int nova_aabb_overlap(nova_aabb a, nova_aabb b)
{
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

int nova_aabb_sweep(nova_aabb a, int vx, int vy, nova_aabb b, int *tx, int *ty)
{
    /* very small conservative swept test: step the moving box and report the
       first integer time of overlap (scaled by 16). */
    for (int step = 0; step <= 16; step++) {
        nova_aabb moved = a;
        moved.x += vx * step / 16;
        moved.y += vy * step / 16;
        if (nova_aabb_overlap(moved, b)) {
            if (tx) *tx = moved.x;
            if (ty) *ty = moved.y;
            return step;
        }
    }
    return -1;
}

static unsigned cell_hash(int cx, int cy, int nbuckets)
{
    unsigned h = (unsigned)(cx * 73856093) ^ (unsigned)(cy * 19349663);
    return h % (unsigned)nbuckets;
}

int nova_spatial_init(nova_spatial *s, int cell, int nbuckets, int capitems)
{
    memset(s, 0, sizeof(*s));
    if (cell <= 0 || nbuckets <= 0 || capitems <= 0) return -1;
    s->cell = cell;
    s->nbuckets = nbuckets;
    s->capitems = capitems;
    s->buckets = (int*)malloc(sizeof(int) * nbuckets);
    s->next = (int*)malloc(sizeof(int) * capitems);
    s->boxes = (nova_aabb*)malloc(sizeof(nova_aabb) * capitems);
    if (!s->buckets || !s->next || !s->boxes) { nova_spatial_free(s); return -1; }
    for (int i = 0; i < nbuckets; i++) s->buckets[i] = -1;
    s->nitems = 0;
    return 0;
}

int nova_spatial_insert(nova_spatial *s, nova_aabb box)
{
    if (s->nitems >= s->capitems) return -1;
    int id = s->nitems++;
    s->boxes[id] = box;
    int cx = box.x / s->cell, cy = box.y / s->cell;
    unsigned b = cell_hash(cx, cy, s->nbuckets);
    s->next[id] = s->buckets[b];
    s->buckets[b] = id;
    return id;
}

int nova_spatial_query(nova_spatial *s, nova_aabb box, int *out, int outcap)
{
    int found = 0;
    int x0 = box.x / s->cell, y0 = box.y / s->cell;
    int x1 = (box.x + box.w) / s->cell, y1 = (box.y + box.h) / s->cell;
    for (int cy = y0; cy <= y1; cy++) {
        for (int cx = x0; cx <= x1; cx++) {
            unsigned b = cell_hash(cx, cy, s->nbuckets);
            for (int id = s->buckets[b]; id != -1; id = s->next[id]) {
                if (nova_aabb_overlap(box, s->boxes[id])) {
                    if (found < outcap) out[found] = id;
                    found++;
                }
            }
        }
    }
    return found;
}

void nova_spatial_free(nova_spatial *s)
{
    free(s->buckets);
    free(s->next);
    free(s->boxes);
    s->buckets = NULL; s->next = NULL; s->boxes = NULL;
    s->nitems = s->capitems = 0;
}
