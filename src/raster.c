#include "nova.h"
#include <stdlib.h>

/* clipped software rasterizer over the palette-index framebuffer.
   every primitive bounds-checks against the framebuffer extent. */

static inline int in_bounds(int x, int y)
{
    return x >= 0 && y >= 0 && x < NOVA_FB_W && y < NOVA_FB_H;
}

static inline void put(nova_machine *mc, int x, int y, uint8_t c)
{
    if (!mc->gfx.fb) return;
    if (in_bounds(x, y))
        mc->gfx.fb[y * NOVA_FB_W + x] = c;
}

void nova_raster_clear(nova_machine *mc, uint8_t color)
{
    if (!mc->gfx.fb) return;
    for (int i = 0; i < NOVA_FB_SIZE; i++)
        mc->gfx.fb[i] = color;
}

void nova_raster_pixel(nova_machine *mc, int x, int y, uint8_t color)
{
    put(mc, x - mc->gfx.cam_x, y - mc->gfx.cam_y, color);
}

void nova_raster_hline(nova_machine *mc, int x0, int x1, int y, uint8_t color)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (x0 < 0) x0 = 0;
    if (x1 >= NOVA_FB_W) x1 = NOVA_FB_W - 1;
    if (y < 0 || y >= NOVA_FB_H || !mc->gfx.fb) return;
    for (int x = x0; x <= x1; x++)
        mc->gfx.fb[y * NOVA_FB_W + x] = color;
}

void nova_raster_vline(nova_machine *mc, int x, int y0, int y1, uint8_t color)
{
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (y0 < 0) y0 = 0;
    if (y1 >= NOVA_FB_H) y1 = NOVA_FB_H - 1;
    if (x < 0 || x >= NOVA_FB_W || !mc->gfx.fb) return;
    for (int y = y0; y <= y1; y++)
        mc->gfx.fb[y * NOVA_FB_W + x] = color;
}

void nova_raster_line(nova_machine *mc, int x0, int y0, int x1, int y1, uint8_t color)
{
    int dx = abs(x1 - x0), dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int guard = NOVA_FB_W * NOVA_FB_H + 4;
    while (guard-- > 0) {
        put(mc, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void nova_raster_rect(nova_machine *mc, int x, int y, int w, int h, uint8_t color)
{
    if (w <= 0 || h <= 0) return;
    nova_raster_hline(mc, x, x + w - 1, y, color);
    nova_raster_hline(mc, x, x + w - 1, y + h - 1, color);
    nova_raster_vline(mc, x, y, y + h - 1, color);
    nova_raster_vline(mc, x + w - 1, y, y + h - 1, color);
}

void nova_raster_fill(nova_machine *mc, int x, int y, int w, int h, uint8_t color)
{
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++)
        nova_raster_hline(mc, x, x + w - 1, y + j, color);
}

void nova_raster_circle(nova_machine *mc, int cx, int cy, int r, uint8_t color)
{
    if (r < 0) return;
    int x = r, y = 0, err = 1 - r;
    int guard = 4 * (r + 1) + 8;
    while (x >= y && guard-- > 0) {
        put(mc, cx + x, cy + y, color); put(mc, cx + y, cy + x, color);
        put(mc, cx - y, cy + x, color); put(mc, cx - x, cy + y, color);
        put(mc, cx - x, cy - y, color); put(mc, cx - y, cy - x, color);
        put(mc, cx + y, cy - x, color); put(mc, cx + x, cy - y, color);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

static uint8_t blend_px(uint8_t dst, uint8_t src, int mode)
{
    switch (mode) {
    case NOVA_BLEND_ADD: { int v = dst + src; return (uint8_t)(v > 255 ? 255 : v); }
    case NOVA_BLEND_SUB: { int v = dst - src; return (uint8_t)(v < 0 ? 0 : v); }
    case NOVA_BLEND_OR:  return (uint8_t)(dst | src);
    default:             return src;
    }
}

void nova_raster_blit_scaled(nova_machine *mc, int id, int x, int y, int sx, int sy, int blend)
{
    if (id < 0 || id >= mc->spr.nbanks) return;
    nova_bank_img *b = &mc->spr.banks[id];
    if (!b->pixels || !mc->gfx.fb) return;
    if (sx <= 0) sx = 1;
    if (sy <= 0) sy = 1;
    int dw = b->w * sx, dh = b->h * sy;
    for (int j = 0; j < dh; j++) {
        int srcy = j / sy;
        if (srcy >= b->h) srcy = b->h - 1;
        int py = y + j - mc->gfx.cam_y;
        if (py < 0 || py >= NOVA_FB_H) continue;
        for (int i = 0; i < dw; i++) {
            int srcx = i / sx;
            if (srcx >= b->w) srcx = b->w - 1;
            int px = x + i - mc->gfx.cam_x;
            if (px < 0 || px >= NOVA_FB_W) continue;
            uint8_t s = b->pixels[srcy * b->w + srcx];
            uint8_t *d = &mc->gfx.fb[py * NOVA_FB_W + px];
            *d = blend_px(*d, s, blend);
        }
    }
}
