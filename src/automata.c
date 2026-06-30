#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* generic life-like cellular automata over a grid plus a cave generator that
   smooths random noise into connected caverns. cells are 0 or 1. */

static int neighbours(nova_grid *g, int x, int y)
{
    int c = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || ny < 0 || nx >= g->w || ny >= g->h) { c++; continue; }
            if (nova_grid_get(g, nx, ny)) c++;
        }
    return c;
}

int nova_ca_step(nova_grid *g, int birth, int survive)
{
    if (!g->cells) return -1;
    int n = g->w * g->h;
    uint8_t *next = (uint8_t*)malloc((size_t)n);
    if (!next) return -1;
    int changed = 0;
    for (int y = 0; y < g->h; y++) {
        for (int x = 0; x < g->w; x++) {
            int cnt = neighbours(g, x, y);
            int alive = nova_grid_get(g, x, y);
            int nv = alive ? ((survive >> cnt) & 1) : ((birth >> cnt) & 1);
            next[y * g->w + x] = (uint8_t)nv;
            if (nv != alive) changed++;
        }
    }
    memcpy(g->cells, next, (size_t)n);
    free(next);
    return changed;
}

int nova_ca_cave(nova_grid *g, uint32_t seed, int iters, int fill_pct)
{
    if (!g->cells) return -1;
    if (fill_pct < 0) fill_pct = 0;
    if (fill_pct > 100) fill_pct = 100;
    nova_rng r;
    nova_rng_seed(&r, seed);
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++)
            nova_grid_set(g, x, y, (uint8_t)((int)(nova_rng_next(&r) % 100) < fill_pct ? 1 : 0));

    /* classic 4-5 rule smoothing */
    int birth = 0, survive = 0;
    for (int k = 5; k <= 8; k++) birth |= (1 << k);
    for (int k = 4; k <= 8; k++) survive |= (1 << k);
    if (iters < 0) iters = 0;
    if (iters > 32) iters = 32;
    for (int i = 0; i < iters; i++) nova_ca_step(g, birth, survive);

    int floor = 0;
    for (int i = 0; i < g->w * g->h; i++) if (!g->cells[i]) floor++;
    return floor;
}
