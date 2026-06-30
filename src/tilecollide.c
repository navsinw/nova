#include "nova.h"

/* resolve a moving AABB (in pixels) against solid cells of a tile grid.
   tiles are 8x8; a cell equal to `tile` is solid. axis-separated sweep. */

#define TILE_SZ 8

static int solid(const uint8_t *grid, int w, int h, int tile, int cx, int cy)
{
    if (cx < 0 || cy < 0 || cx >= w || cy >= h) return 1;   /* world edge is solid */
    return grid[cy * w + cx] == (uint8_t)tile;
}

static int box_hits(const uint8_t *grid, int w, int h, int tile, int x, int y, int bw, int bh)
{
    int x0 = x / TILE_SZ, y0 = y / TILE_SZ;
    int x1 = (x + bw - 1) / TILE_SZ, y1 = (y + bh - 1) / TILE_SZ;
    for (int cy = y0; cy <= y1; cy++)
        for (int cx = x0; cx <= x1; cx++)
            if (solid(grid, w, h, tile, cx, cy)) return 1;
    return 0;
}

int nova_tile_resolve(const uint8_t *grid, int w, int h, int tile,
                      int *x, int *y, int bw, int bh, int vx, int vy)
{
    if (!grid || !x || !y || w <= 0 || h <= 0) return 0;
    int hit = 0;

    int nx = *x + vx;
    if (box_hits(grid, w, h, tile, nx, *y, bw, bh)) {
        int step = vx > 0 ? 1 : -1;
        while (vx != 0 && !box_hits(grid, w, h, tile, *x + step, *y, bw, bh)) *x += step, vx -= step;
        hit |= 1;
    } else {
        *x = nx;
    }

    int ny = *y + vy;
    if (box_hits(grid, w, h, tile, *x, ny, bw, bh)) {
        int step = vy > 0 ? 1 : -1;
        while (vy != 0 && !box_hits(grid, w, h, tile, *x, *y + step, bw, bh)) *y += step, vy -= step;
        hit |= 2;
    } else {
        *y = ny;
    }

    return hit;
}
