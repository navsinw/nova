#include "nova.h"

/* colour and palette manipulation in 0xAARRGGBB space. */

uint32_t nova_rgb_blend(uint32_t a, uint32_t b, int t)
{
    if (t < 0) t = 0;
    if (t > 256) t = 256;
    int ar = (a >> 16) & 0xff, ag = (a >> 8) & 0xff, ab = a & 0xff;
    int br = (b >> 16) & 0xff, bg = (b >> 8) & 0xff, bb = b & 0xff;
    int r = ar + (br - ar) * t / 256;
    int g = ag + (bg - ag) * t / 256;
    int bl = ab + (bb - ab) * t / 256;
    return 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)bl;
}

uint32_t nova_rgb_scale(uint32_t c, int q8)
{
    int r = ((c >> 16) & 0xff) * q8 >> 8;
    int g = ((c >> 8) & 0xff) * q8 >> 8;
    int b = (c & 0xff) * q8 >> 8;
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void nova_pal_rotate(uint32_t *pal, int n, int amount)
{
    if (!pal || n <= 1) return;
    amount = ((amount % n) + n) % n;
    if (amount == 0) return;
    uint32_t tmp[256];
    if (n > 256) n = 256;
    for (int i = 0; i < n; i++) tmp[i] = pal[i];
    for (int i = 0; i < n; i++) pal[(i + amount) % n] = tmp[i];
}

void nova_pal_gradient(uint32_t *pal, int n, uint32_t a, uint32_t b)
{
    if (!pal || n <= 0) return;
    for (int i = 0; i < n; i++) {
        int t = n > 1 ? i * 256 / (n - 1) : 0;
        pal[i] = nova_rgb_blend(a, b, t);
    }
}
