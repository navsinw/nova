#include "nova.h"
#include <stdlib.h>

/* A* over a tile grid. grid cells: 0 walkable, nonzero blocked.
   writes the path (start..goal) as x,y pairs into out_xy. */

static int heur(int ax, int ay, int bx, int by)
{
    int dx = ax - bx, dy = ay - by;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx + dy;
}

int nova_path_find(const uint8_t *grid, int w, int h,
                   int sx, int sy, int dx, int dy,
                   int *out_xy, int out_cap)
{
    if (!grid || w <= 0 || h <= 0) return 0;
    if ((long)w * h > (1 << 20)) return 0;
    if (sx < 0 || sy < 0 || sx >= w || sy >= h) return 0;
    if (dx < 0 || dy < 0 || dx >= w || dy >= h) return 0;
    if (grid[sy * w + sx] || grid[dy * w + dx]) return 0;

    int n = w * h;
    int *gcost = (int*)malloc(sizeof(int) * n);
    int *fcost = (int*)malloc(sizeof(int) * n);
    int *came = (int*)malloc(sizeof(int) * n);
    uint8_t *open = (uint8_t*)calloc(n, 1);
    uint8_t *closed = (uint8_t*)calloc(n, 1);
    if (!gcost || !fcost || !came || !open || !closed) {
        free(gcost); free(fcost); free(came); free(open); free(closed);
        return 0;
    }
    for (int i = 0; i < n; i++) { gcost[i] = 0x3fffffff; fcost[i] = 0x3fffffff; came[i] = -1; }

    int start = sy * w + sx, goal = dy * w + dx;
    gcost[start] = 0;
    fcost[start] = heur(sx, sy, dx, dy);
    open[start] = 1;

    int result = 0;
    int iter = 0, maxiter = n * 4 + 16;
    while (iter++ < maxiter) {
        int best = -1, bestf = 0x7fffffff;
        for (int i = 0; i < n; i++)
            if (open[i] && fcost[i] < bestf) { bestf = fcost[i]; best = i; }
        if (best < 0) break;
        if (best == goal) {
            /* reconstruct */
            int len = 0, c = goal;
            while (c != -1) { len++; c = came[c]; }
            if (len * 2 <= out_cap && out_xy) {
                int idx = len - 1; c = goal;
                while (c != -1) {
                    out_xy[idx * 2] = c % w;
                    out_xy[idx * 2 + 1] = c / w;
                    idx--; c = came[c];
                }
            }
            result = len;
            break;
        }
        open[best] = 0;
        closed[best] = 1;
        int bx = best % w, by = best / w;
        static const int ox[4] = { 1, -1, 0, 0 };
        static const int oy[4] = { 0, 0, 1, -1 };
        for (int k = 0; k < 4; k++) {
            int nx = bx + ox[k], ny = by + oy[k];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
            int ni = ny * w + nx;
            if (closed[ni] || grid[ni]) continue;
            int tg = gcost[best] + 1;
            if (tg < gcost[ni]) {
                came[ni] = best;
                gcost[ni] = tg;
                fcost[ni] = tg + heur(nx, ny, dx, dy);
                open[ni] = 1;
            }
        }
    }

    free(gcost); free(fcost); free(came); free(open); free(closed);
    return result;
}
