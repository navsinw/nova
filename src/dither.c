#include "nova.h"

/* palette-index image post effects: ordered dithering, nearest-colour
   quantization, and a luminance pass. */

static const int bayer4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};

void nova_dither_ordered(uint8_t *fb, int w, int h, int levels)
{
    if (!fb || w <= 0 || h <= 0) return;
    if (levels < 2) levels = 2;
    if (levels > 256) levels = 256;
    int step = 256 / levels;
    if (step < 1) step = 1;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int v = fb[y * w + x];
            int t = bayer4[y & 3][x & 3] * step / 16;
            int q = ((v + t) / step) * step;
            if (q > 255) q = 255;
            fb[y * w + x] = (uint8_t)q;
        }
    }
}

int nova_quantize(const uint32_t *pal, int npal, uint32_t rgb)
{
    if (!pal || npal <= 0) return 0;
    int best = 0;
    long bestd = 0x7fffffffL;
    int r = (rgb >> 16) & 0xff, g = (rgb >> 8) & 0xff, b = rgb & 0xff;
    for (int i = 0; i < npal; i++) {
        int pr = (pal[i] >> 16) & 0xff, pg = (pal[i] >> 8) & 0xff, pb = pal[i] & 0xff;
        long dr = r - pr, dg = g - pg, db = b - pb;
        long d = dr * dr + dg * dg + db * db;
        if (d < bestd) { bestd = d; best = i; }
    }
    return best;
}

void nova_grayscale(uint8_t *fb, int w, int h)
{
    if (!fb || w <= 0 || h <= 0) return;
    for (int i = 0; i < w * h; i++) {
        int v = fb[i];
        /* fold colour index toward a coarse grey ramp */
        fb[i] = (uint8_t)((v & 0xf8));
    }
}
