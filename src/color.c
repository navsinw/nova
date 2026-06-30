#include "nova.h"

/* HSV<->RGB conversion (integer; h in 0..359, s/v in 0..255). */

uint32_t nova_hsv_to_rgb(int h, int s, int v)
{
    h = ((h % 360) + 360) % 360;
    if (s < 0) s = 0; if (s > 255) s = 255;
    if (v < 0) v = 0; if (v > 255) v = 255;
    int region = h / 60;
    int rem = (h % 60) * 255 / 60;
    int p = v * (255 - s) / 255;
    int q = v * (255 - s * rem / 255) / 255;
    int t = v * (255 - s * (255 - rem) / 255) / 255;
    int r, g, b;
    switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
    return 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void nova_rgb_to_hsv(uint32_t rgb, int *h, int *s, int *v)
{
    int r = (rgb >> 16) & 0xff, g = (rgb >> 8) & 0xff, b = rgb & 0xff;
    int mx = r; if (g > mx) mx = g; if (b > mx) mx = b;
    int mn = r; if (g < mn) mn = g; if (b < mn) mn = b;
    int d = mx - mn;
    int hue = 0;
    if (d != 0) {
        if (mx == r) hue = 60 * (g - b) / d;
        else if (mx == g) hue = 120 + 60 * (b - r) / d;
        else hue = 240 + 60 * (r - g) / d;
    }
    hue = ((hue % 360) + 360) % 360;
    if (h) *h = hue;
    if (s) *s = mx == 0 ? 0 : d * 255 / mx;
    if (v) *v = mx;
}

uint32_t nova_color_lerp_hsv(uint32_t a, uint32_t b, int t)
{
    int ha, sa, va, hb, sb, vb;
    nova_rgb_to_hsv(a, &ha, &sa, &va);
    nova_rgb_to_hsv(b, &hb, &sb, &vb);
    if (t < 0) t = 0; if (t > 256) t = 256;
    int h = ha + (hb - ha) * t / 256;
    int s = sa + (sb - sa) * t / 256;
    int v = va + (vb - va) * t / 256;
    return nova_hsv_to_rgb(h, s, v);
}
