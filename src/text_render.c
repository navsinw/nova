#include "nova.h"
#include <string.h>

/* draw font glyphs into the framebuffer. simple (non-composite) glyphs are
   blitted pixel by pixel where the bitmap is non-zero; every write is clipped. */

void nova_font_blit(nova_machine *mc, int glyph, int x, int y, uint8_t color)
{
    if (glyph < 0 || glyph >= mc->font.nglyphs || !mc->gfx.fb) return;
    nova_glyph *g = &mc->font.glyphs[glyph];
    if (g->is_composite || !g->bitmap) return;
    for (int j = 0; j < g->h; j++) {
        int py = y + j - mc->gfx.cam_y;
        if (py < 0 || py >= NOVA_FB_H) continue;
        for (int i = 0; i < g->w; i++) {
            int px = x + i - mc->gfx.cam_x;
            if (px < 0 || px >= NOVA_FB_W) continue;
            if (g->bitmap[j * g->w + i])
                mc->gfx.fb[py * NOVA_FB_W + px] = color;
        }
    }
}

int nova_text_draw(nova_machine *mc, const char *str, int x, int y, uint8_t color)
{
    int cx = x;
    int drawn = 0;
    for (const char *p = str; *p; p++) {
        if (*p == '\n') { cx = x; y += 8; continue; }
        int glyph = (uint8_t)*p;
        int gw = 6;
        if (glyph < mc->font.nglyphs && mc->font.glyphs[glyph].w > 0)
            gw = mc->font.glyphs[glyph].w + 1;
        nova_font_blit(mc, glyph, cx, y, color);
        cx += gw;
        drawn++;
    }
    return drawn;
}

int nova_text_draw_wrapped(nova_machine *mc, const char *str, int x, int y, int width, uint8_t color)
{
    char wrapped[1024];
    nova_text_wrap(str, width, wrapped, sizeof(wrapped));
    return nova_text_draw(mc, wrapped, x, y, color);
}
