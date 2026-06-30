#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* 3x3 convolution filters over the palette-index framebuffer. results are
   written through a temporary copy so neighbours read original values. */

static void convolve(uint8_t *fb, int w, int h, const int *k, int div, int bias)
{
    if (!fb || w <= 0 || h <= 0) return;
    uint8_t *tmp = (uint8_t*)malloc((size_t)w * h);
    if (!tmp) return;
    memcpy(tmp, fb, (size_t)w * h);
    if (div == 0) div = 1;
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            int acc = 0, ki = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    acc += tmp[(y + dy) * w + (x + dx)] * k[ki++];
            int v = acc / div + bias;
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            fb[y * w + x] = (uint8_t)v;
        }
    }
    free(tmp);
}

void nova_filter_blur(uint8_t *fb, int w, int h)
{
    static const int k[9] = { 1,1,1, 1,1,1, 1,1,1 };
    convolve(fb, w, h, k, 9, 0);
}

void nova_filter_sharpen(uint8_t *fb, int w, int h)
{
    static const int k[9] = { 0,-1,0, -1,5,-1, 0,-1,0 };
    convolve(fb, w, h, k, 1, 0);
}

void nova_filter_edges(uint8_t *fb, int w, int h)
{
    static const int k[9] = { -1,-1,-1, -1,8,-1, -1,-1,-1 };
    convolve(fb, w, h, k, 1, 0);
}
