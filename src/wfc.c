#include "nova.h"
#include <stdlib.h>

/* a simplified wave-function-collapse: fill a grid with tile ids such that
   horizontally/vertically adjacent tiles differ by at most one (a band rule).
   greedy with a small candidate set; deterministic for a given seed. */

static int compatible(int a, int b)
{
    int d = a - b;
    if (d < 0) d = -d;
    return d <= 1;
}

int nova_wfc_gen(nova_grid *g, uint32_t seed, int ntiles)
{
    if (!g->cells || ntiles < 1) return -1;
    if (ntiles > 255) ntiles = 255;
    nova_rng r;
    nova_rng_seed(&r, seed);

    int w = g->w, h = g->h;
    int collapsed = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int left = x > 0 ? nova_grid_get(g, x - 1, y) : -1;
            int up = y > 0 ? nova_grid_get(g, x, y - 1) : -1;

            int cand[256], nc = 0;
            for (int t = 0; t < ntiles; t++) {
                int ok = 1;
                if (left >= 0 && !compatible(t, left)) ok = 0;
                if (up >= 0 && !compatible(t, up)) ok = 0;
                if (ok) cand[nc++] = t;
            }
            int chosen;
            if (nc == 0) {
                /* contradiction: relax to the neighbour's value */
                chosen = left >= 0 ? left : (up >= 0 ? up : 0);
            } else {
                chosen = cand[nova_rng_next(&r) % (uint32_t)nc];
                collapsed++;
            }
            nova_grid_set(g, x, y, (uint8_t)chosen);
        }
    }
    return collapsed;
}
