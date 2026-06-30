#include "nova.h"
#include <stdlib.h>
#include <string.h>

static uint16_t g16(const uint8_t *p){ return (uint16_t)(p[0]|(p[1]<<8)); }
static uint32_t g32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

int nova_gfx_init(nova_machine *mc)
{
    memset(&mc->gfx, 0, sizeof(mc->gfx));
    memset(&mc->spr, 0, sizeof(mc->spr));
    mc->gfx.fb = (uint8_t*)calloc(NOVA_FB_SIZE, 1);

    const uint8_t *p; uint32_t n;
    if (nova_cart_find(&mc->cart, TAG_SPRT, &p, &n) != 0) return 0;
    if (n < 8) return 0;

    int cnt = g16(p);
    int draw = g16(p + 2);
    uint32_t anim_cnt = g32(p + 4);
    if (cnt < 0 || cnt > 4096) return 0;

    mc->spr.banks = (nova_bank_img*)calloc(cnt ? cnt : 1, sizeof(nova_bank_img));
    mc->spr.nbanks = cnt;
    mc->spr.draw_cnt = draw;

    uint32_t off = 8;
    for (int i = 0; i < cnt; i++) {
        if (off + 4 > n) { mc->spr.nbanks = i; break; }
        int w = g16(p + off), h = g16(p + off + 2);
        off += 4;
        if (w <= 0 || h <= 0 || w > 256 || h > 256) continue;
        uint32_t px = (uint32_t)w * (uint32_t)h;
        if (off + px > n) { mc->spr.nbanks = i; break; }
        mc->spr.banks[i].pixels = (uint8_t*)malloc(px);
        memcpy(mc->spr.banks[i].pixels, p + off, px);
        mc->spr.banks[i].w = (uint16_t)w;
        mc->spr.banks[i].h = (uint16_t)h;
        off += px;
    }

    uint32_t abytes = anim_cnt * (uint32_t)sizeof(uint16_t);
    mc->spr.anim = (uint16_t*)malloc(abytes ? abytes : 2);
    mc->spr.anim_cnt = (int)anim_cnt;
    for (uint32_t i = 0; i < anim_cnt; i++)
        mc->spr.anim[i] = (uint16_t)(i & 0xffff);

    return 0;
}

void nova_gfx_free(nova_machine *mc)
{
    for (int i = 0; i < mc->spr.nbanks; i++)
        free(mc->spr.banks[i].pixels);
    free(mc->spr.banks);
    free(mc->spr.anim);
    free(mc->gfx.fb);
    mc->spr.banks = NULL;
    mc->spr.anim = NULL;
    mc->gfx.fb = NULL;
}

static void blit(nova_machine *mc, nova_bank_img *b, int x, int y)
{
    if (!b->pixels) return;
    for (int j = 0; j < b->h; j++) {
        for (int i = 0; i < b->w; i++) {
            int px = x + i - mc->gfx.cam_x;
            int py = y + j - mc->gfx.cam_y;
            if (px < 0 || py < 0 || px >= NOVA_FB_W || py >= NOVA_FB_H) continue;
            mc->gfx.fb[py * NOVA_FB_W + px] = b->pixels[j * b->w + i];
        }
    }
}

void nova_gfx_sprite(nova_machine *mc, int id, int x, int y)
{
    if (id < 0 || id >= mc->spr.draw_cnt) return;
    nova_bank_img *b = &mc->spr.banks[id];
    if (!b->pixels) return;

    int dx = x - mc->gfx.cam_x;
    int dy = y - mc->gfx.cam_y;
    if (b->w >= 16) {
        for (int j = 0; j < b->h; j++) {
            int py = dy + j;
            if (py < 0 || py >= NOVA_FB_H) continue;
            for (int i = 0; i < b->w; i++)
                mc->gfx.fb[py * NOVA_FB_W + dx + i] = b->pixels[j * b->w + i];
        }
        return;
    }
    blit(mc, b, x, y);
}

void nova_gfx_tile(nova_machine *mc, int mx, int my, int cols, int rows)
{
    if (!mc->map.tiles || !mc->spr.banks) return;
    int tw = mc->map.tile_w > 0 ? mc->map.tile_w : 8;
    int th = mc->map.tile_h > 0 ? mc->map.tile_h : 8;
    for (int ry = 0; ry < rows; ry++) {
        for (int rx = 0; rx < cols; rx++) {
            int cx = mx + rx, cy = my + ry;
            if (cx < 0 || cy < 0 || cx >= mc->map.w || cy >= mc->map.h) continue;
            int tile = mc->map.tiles[cy * mc->map.w + cx];
            nova_bank_img *b = &mc->spr.banks[tile];
            blit(mc, b, rx * tw, ry * th);
        }
    }
}

void nova_gfx_text(nova_machine *mc, uint32_t straddr, int x, int y)
{
    (void)x; (void)y;
    uint8_t tmp[512];
    int n = 0;
    while (n < 512) {
        int32_t c = nova_mem_load(&mc->mem, straddr + (uint32_t)n);
        if (c == 0) break;
        tmp[n++] = (uint8_t)c;
    }
    nova_font_render(mc, tmp, n);
}
