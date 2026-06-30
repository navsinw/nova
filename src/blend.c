#include "nova.h"

/* framebuffer-to-framebuffer compositing for layered rendering. */

void nova_fb_blend(uint8_t *dst, const uint8_t *src, int n, int mode)
{
    if (!dst || !src || n <= 0) return;
    for (int i = 0; i < n; i++) {
        int d = dst[i], s = src[i], v;
        switch (mode) {
        case NOVA_BLEND_ADD: v = d + s; if (v > 255) v = 255; break;
        case NOVA_BLEND_SUB: v = d - s; if (v < 0) v = 0; break;
        case NOVA_BLEND_OR:  v = d | s; break;
        default:             v = s ? s : d; break;   /* copy non-transparent */
        }
        dst[i] = (uint8_t)v;
    }
}

void nova_fb_mask(uint8_t *dst, const uint8_t *src, const uint8_t *mask, int n)
{
    if (!dst || !src || !mask || n <= 0) return;
    for (int i = 0; i < n; i++)
        if (mask[i]) dst[i] = src[i];
}

void nova_fb_fade(uint8_t *fb, int n, int amount)
{
    if (!fb || n <= 0) return;
    if (amount < 0) amount = 0;
    if (amount > 255) amount = 255;
    for (int i = 0; i < n; i++) {
        int v = fb[i] - amount;
        if (v < 0) v = 0;
        fb[i] = (uint8_t)v;
    }
}
