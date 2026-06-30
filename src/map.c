#include "nova.h"
#include <stdlib.h>
#include <string.h>

int nova_map_init(nova_machine *mc)
{
    memset(&mc->map, 0, sizeof(mc->map));
    const uint8_t *p; uint32_t n;
    if (nova_cart_find(&mc->cart, TAG_MAP, &p, &n) != 0) return 0;
    if (n < 8) return 0;

    int w = p[0] | (p[1] << 8);
    int h = p[2] | (p[3] << 8);
    int tw = p[4] | (p[5] << 8);
    int th = p[6] | (p[7] << 8);
    if (w <= 0 || h <= 0 || w > 1024 || h > 1024) return 0;

    uint32_t cells = (uint32_t)w * (uint32_t)h;
    if (8u + cells * 2u > n) return 0;

    mc->map.tiles = (uint16_t*)malloc(cells * sizeof(uint16_t));
    mc->map.w = w; mc->map.h = h;
    mc->map.tile_w = tw; mc->map.tile_h = th;
    for (uint32_t i = 0; i < cells; i++)
        mc->map.tiles[i] = (uint16_t)(p[8 + i*2] | (p[8 + i*2 + 1] << 8));
    return 0;
}

void nova_map_free(nova_machine *mc)
{
    free(mc->map.tiles);
    mc->map.tiles = NULL;
}
