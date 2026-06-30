#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* generic byte grid with a bounded scanline flood fill. */

int nova_grid_init(nova_grid *g, int w, int h)
{
    if (w <= 0 || h <= 0 || (long)w * h > (1 << 22)) return -1;
    g->cells = (uint8_t*)calloc((size_t)w * h, 1);
    if (!g->cells) return -1;
    g->w = w; g->h = h;
    return 0;
}

void nova_grid_set(nova_grid *g, int x, int y, uint8_t v)
{
    if (!g->cells || x < 0 || y < 0 || x >= g->w || y >= g->h) return;
    g->cells[y * g->w + x] = v;
}

uint8_t nova_grid_get(const nova_grid *g, int x, int y)
{
    if (!g->cells || x < 0 || y < 0 || x >= g->w || y >= g->h) return 0;
    return g->cells[y * g->w + x];
}

void nova_grid_fill(nova_grid *g, uint8_t v)
{
    if (!g->cells) return;
    memset(g->cells, v, (size_t)g->w * g->h);
}

int nova_grid_flood(nova_grid *g, int x, int y, uint8_t from, uint8_t to)
{
    if (!g->cells || from == to) return 0;
    if (x < 0 || y < 0 || x >= g->w || y >= g->h) return 0;
    if (nova_grid_get(g, x, y) != from) return 0;

    int cap = g->w * g->h;
    int *stack = (int*)malloc(sizeof(int) * cap);
    if (!stack) return 0;
    int sp = 0, filled = 0;
    stack[sp++] = y * g->w + x;
    while (sp > 0) {
        int idx = stack[--sp];
        if (g->cells[idx] != from) continue;
        g->cells[idx] = to;
        filled++;
        int cx = idx % g->w, cy = idx / g->w;
        if (cx > 0)        stack[sp++] = idx - 1;
        if (cx < g->w - 1) stack[sp++] = idx + 1;
        if (cy > 0)        stack[sp++] = idx - g->w;
        if (cy < g->h - 1) stack[sp++] = idx + g->w;
        if (sp > cap - 4) break;
    }
    free(stack);
    return filled;
}

void nova_grid_free(nova_grid *g)
{
    free(g->cells);
    g->cells = NULL;
    g->w = g->h = 0;
}
