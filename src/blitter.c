#include "nova.h"

/* affine sprite drawing: rotate/scale a sprite bank into the framebuffer by
   inverse-mapping each destination pixel back into source space. fully clipped. */

void nova_blit_affine(nova_machine *mc, int id, int cx, int cy, int angle, int scale)
{
    if (id < 0 || id >= mc->spr.nbanks || !mc->gfx.fb) return;
    nova_bank_img *b = &mc->spr.banks[id];
    if (!b->pixels) return;
    if (scale <= 0) scale = NOVA_FP_ONE;

    int32_t ca = nova_fp_cos(angle << 8);
    int32_t sa = nova_fp_sin(angle << 8);
    /* inverse scale to map dest->source */
    int32_t inv = nova_fp_div(NOVA_FP_ONE, scale);

    int half = (b->w > b->h ? b->w : b->h) * scale / NOVA_FP_ONE;
    if (half < 1) half = 1;
    if (half > 256) half = 256;

    for (int dy = -half; dy <= half; dy++) {
        int py = cy + dy - mc->gfx.cam_y;
        if (py < 0 || py >= NOVA_FB_H) continue;
        for (int dx = -half; dx <= half; dx++) {
            int px = cx + dx - mc->gfx.cam_x;
            if (px < 0 || px >= NOVA_FB_W) continue;
            int32_t fx = nova_fp_mul((int32_t)dx << 16, inv);
            int32_t fy = nova_fp_mul((int32_t)dy << 16, inv);
            int32_t sxp = nova_fp_mul(fx, ca) - nova_fp_mul(fy, sa);
            int32_t syp = nova_fp_mul(fx, sa) + nova_fp_mul(fy, ca);
            int sx = (sxp >> 16) + b->w / 2;
            int sy = (syp >> 16) + b->h / 2;
            if (sx < 0 || sy < 0 || sx >= b->w || sy >= b->h) continue;
            uint8_t c = b->pixels[sy * b->w + sx];
            if (c) mc->gfx.fb[py * NOVA_FB_W + px] = c;
        }
    }
}

void nova_blit_rotozoom(nova_machine *mc, int id, int x, int y, int angle, int sx, int sy)
{
    (void)sy;
    nova_blit_affine(mc, id, x, y, angle, sx > 0 ? sx : NOVA_FP_ONE);
}
