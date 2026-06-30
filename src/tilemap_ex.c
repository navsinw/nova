#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* multi-layer scrolling tilemap with 8-neighbour autotiling. drawn through the
   sprite path with per-layer scroll offsets. */

int nova_tmx_init(nova_tilemap_ex *t, int w, int h, int layers)
{
    memset(t, 0, sizeof(*t));
    if (w <= 0 || h <= 0 || w > 1024 || h > 1024) return -1;
    if (layers < 1) layers = 1;
    if (layers > NOVA_TM_LAYERS) layers = NOVA_TM_LAYERS;
    t->w = w; t->h = h; t->nlayers = layers;
    for (int i = 0; i < layers; i++) {
        t->cells[i] = (uint16_t*)calloc((size_t)w * h, sizeof(uint16_t));
        if (!t->cells[i]) return -1;
    }
    return 0;
}

void nova_tmx_scroll(nova_tilemap_ex *t, int layer, int dx, int dy)
{
    if (layer < 0 || layer >= t->nlayers) return;
    t->scroll_x[layer] += dx;
    t->scroll_y[layer] += dy;
}

static int solid_at(nova_tilemap_ex *t, int layer, int x, int y)
{
    if (x < 0 || y < 0 || x >= t->w || y >= t->h) return 0;
    return t->cells[layer][y * t->w + x] != 0;
}

int nova_tmx_autotile(nova_tilemap_ex *t, int layer, int x, int y)
{
    if (layer < 0 || layer >= t->nlayers) return 0;
    if (x < 0 || y < 0 || x >= t->w || y >= t->h) return 0;
    int mask = 0;
    if (solid_at(t, layer, x, y - 1)) mask |= 1;
    if (solid_at(t, layer, x + 1, y)) mask |= 2;
    if (solid_at(t, layer, x, y + 1)) mask |= 4;
    if (solid_at(t, layer, x - 1, y)) mask |= 8;
    return mask;
}

void nova_tmx_draw(nova_machine *mc, nova_tilemap_ex *t)
{
    int tw = 8, th = 8;
    for (int l = 0; l < t->nlayers; l++) {
        if (!t->cells[l]) continue;
        int ox = t->scroll_x[l] / tw;
        int oy = t->scroll_y[l] / th;
        for (int cy = 0; cy < NOVA_FB_H / th + 1; cy++) {
            for (int cx = 0; cx < NOVA_FB_W / tw + 1; cx++) {
                int mx = cx + ox, my = cy + oy;
                if (mx < 0 || my < 0 || mx >= t->w || my >= t->h) continue;
                uint16_t tile = t->cells[l][my * t->w + mx];
                if (tile == 0) continue;
                nova_gfx_sprite(mc, tile, cx * tw - (t->scroll_x[l] % tw),
                                cy * th - (t->scroll_y[l] % th));
            }
        }
    }
}

void nova_tmx_free(nova_tilemap_ex *t)
{
    for (int i = 0; i < t->nlayers; i++) free(t->cells[i]);
    memset(t, 0, sizeof(*t));
}
