#include "nova.h"

/* draw a scaled-down overview of a tilemap into a corner of the framebuffer.
   each tile becomes a `scale`-sized block coloured by its tile id. */

void nova_minimap_draw(nova_machine *mc, const uint16_t *tiles, int w, int h, int ox, int oy, int scale)
{
    if (!tiles || !mc->gfx.fb || w <= 0 || h <= 0) return;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    for (int ty = 0; ty < h; ty++) {
        for (int tx = 0; tx < w; tx++) {
            uint16_t tile = tiles[ty * w + tx];
            if (tile == 0) continue;
            uint8_t color = (uint8_t)(tile & 0xff);
            for (int j = 0; j < scale; j++) {
                int py = oy + ty * scale + j;
                if (py < 0 || py >= NOVA_FB_H) continue;
                for (int i = 0; i < scale; i++) {
                    int px = ox + tx * scale + i;
                    if (px < 0 || px >= NOVA_FB_W) continue;
                    mc->gfx.fb[py * NOVA_FB_W + px] = color;
                }
            }
        }
    }
}
