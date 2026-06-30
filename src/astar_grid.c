#include "nova.h"
#include <stdlib.h>

/* weighted A*: each cell carries a movement cost (0 = impassable). distinct
   from path.c, which treats the grid as a simple binary maze. */

static int heur(int ax, int ay, int bx, int by)
{
    int dx = ax - bx, dy = ay - by;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx + dy;
}

int nova_astar_cost(const uint8_t *cost, int w, int h, int sx, int sy, int dx, int dy, int *out_xy, int outcap)
{
    if (!cost || w <= 0 || h <= 0) return 0;
    if ((long)w * h > (1 << 20)) return 0;
    if (sx < 0 || sy < 0 || sx >= w || sy >= h) return 0;
    if (dx < 0 || dy < 0 || dx >= w || dy >= h) return 0;
    if (cost[sy*w+sx] == 0 || cost[dy*w+dx] == 0) return 0;

    int n = w * h;
    int *g = malloc(sizeof(int)*n), *f = malloc(sizeof(int)*n), *came = malloc(sizeof(int)*n);
    uint8_t *open = calloc(n, 1), *closed = calloc(n, 1);
    if (!g || !f || !came || !open || !closed) { free(g);free(f);free(came);free(open);free(closed); return 0; }
    for (int i = 0; i < n; i++) { g[i] = 0x3fffffff; f[i] = 0x3fffffff; came[i] = -1; }

    int start = sy*w+sx, goal = dy*w+dx;
    g[start] = 0; f[start] = heur(sx,sy,dx,dy); open[start] = 1;

    int result = 0, iter = 0, maxiter = n*4+16;
    static const int ox[4] = {1,-1,0,0}, oy[4] = {0,0,1,-1};
    while (iter++ < maxiter) {
        int best = -1, bestf = 0x7fffffff;
        for (int i = 0; i < n; i++) if (open[i] && f[i] < bestf) { bestf = f[i]; best = i; }
        if (best < 0) break;
        if (best == goal) {
            int len = 0, c = goal;
            while (c != -1) { len++; c = came[c]; }
            if (len*2 <= outcap && out_xy) {
                int idx = len-1; c = goal;
                while (c != -1) { out_xy[idx*2] = c % w; out_xy[idx*2+1] = c / w; idx--; c = came[c]; }
            }
            result = len;
            break;
        }
        open[best] = 0; closed[best] = 1;
        int bx = best % w, by = best / w;
        for (int k = 0; k < 4; k++) {
            int nx = bx+ox[k], ny = by+oy[k];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
            int ni = ny*w+nx;
            if (closed[ni] || cost[ni] == 0) continue;
            int tg = g[best] + cost[ni];
            if (tg < g[ni]) { came[ni] = best; g[ni] = tg; f[ni] = tg + heur(nx,ny,dx,dy); open[ni] = 1; }
        }
    }
    free(g); free(f); free(came); free(open); free(closed);
    return result;
}
