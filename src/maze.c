#include "nova.h"
#include <stdlib.h>

/* recursive-backtracker maze carved into a grid: 1 = wall, 0 = passage. */

int nova_maze_gen(nova_grid *g, uint32_t seed)
{
    if (!g->cells || g->w < 3 || g->h < 3) return -1;
    nova_grid_fill(g, 1);
    nova_rng r;
    nova_rng_seed(&r, seed);

    int w = g->w, h = g->h;
    int *stack = (int*)malloc(sizeof(int) * w * h);
    if (!stack) return -1;
    int sp = 0;
    int sx = 1, sy = 1;
    nova_grid_set(g, sx, sy, 0);
    stack[sp++] = sy * w + sx;

    static const int dx[4] = { 0, 2, 0, -2 };
    static const int dy[4] = { -2, 0, 2, 0 };

    while (sp > 0) {
        int idx = stack[sp - 1];
        int cx = idx % w, cy = idx / w;
        int order[4] = { 0, 1, 2, 3 };
        nova_rng_shuffle(&r, order, 4);
        int moved = 0;
        for (int k = 0; k < 4; k++) {
            int nx = cx + dx[order[k]], ny = cy + dy[order[k]];
            if (nx > 0 && ny > 0 && nx < w - 1 && ny < h - 1 && nova_grid_get(g, nx, ny) == 1) {
                nova_grid_set(g, (cx + nx) / 2, (cy + ny) / 2, 0);
                nova_grid_set(g, nx, ny, 0);
                if (sp < w * h) stack[sp++] = ny * w + nx;
                moved = 1;
                break;
            }
        }
        if (!moved) sp--;
    }
    free(stack);
    return 0;
}
