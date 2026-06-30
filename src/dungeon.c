#include "nova.h"
#include <stdlib.h>

/* room-and-corridor dungeon generator. carves rectangular rooms into a wall
   grid and joins their centres with L-shaped corridors. */

static void carve_room(nova_grid *g, int x, int y, int w, int h)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            nova_grid_set(g, x + i, y + j, 0);
}

static void carve_h(nova_grid *g, int x0, int x1, int y)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; x++) nova_grid_set(g, x, y, 0);
}

static void carve_v(nova_grid *g, int y0, int y1, int x)
{
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    for (int y = y0; y <= y1; y++) nova_grid_set(g, x, y, 0);
}

int nova_dungeon_gen(nova_grid *g, uint32_t seed, int max_rooms)
{
    if (!g->cells || g->w < 8 || g->h < 8) return -1;
    if (max_rooms < 1) max_rooms = 1;
    if (max_rooms > 64) max_rooms = 64;
    nova_grid_fill(g, 1);
    nova_rng r;
    nova_rng_seed(&r, seed);

    int rcx[64], rcy[64], nrooms = 0;
    for (int attempt = 0; attempt < max_rooms * 4 && nrooms < max_rooms; attempt++) {
        int rw = nova_rng_range(&r, 3, 8);
        int rh = nova_rng_range(&r, 3, 8);
        int rx = nova_rng_range(&r, 1, g->w - rw - 1);
        int ry = nova_rng_range(&r, 1, g->h - rh - 1);
        if (rx < 1 || ry < 1) continue;

        /* reject if it touches an existing floor (keep rooms separated) */
        int clash = 0;
        for (int j = -1; j <= rh && !clash; j++)
            for (int i = -1; i <= rw; i++)
                if (nova_grid_get(g, rx + i, ry + j) == 0) { clash = 1; break; }
        if (clash) continue;

        carve_room(g, rx, ry, rw, rh);
        int cx = rx + rw / 2, cy = ry + rh / 2;
        if (nrooms > 0) {
            int px = rcx[nrooms - 1], py = rcy[nrooms - 1];
            if (nova_rng_next(&r) & 1) { carve_h(g, px, cx, py); carve_v(g, py, cy, cx); }
            else                       { carve_v(g, py, cy, px); carve_h(g, px, cx, cy); }
        }
        rcx[nrooms] = cx; rcy[nrooms] = cy;
        nrooms++;
    }
    return nrooms;
}
