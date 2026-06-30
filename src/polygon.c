#include "nova.h"

/* polygon point-in-test (even-odd) and scanline fill via the rasterizer. */

int nova_poly_contains(const int *xy, int n, int px, int py)
{
    int inside = 0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        int xi = xy[i*2], yi = xy[i*2+1];
        int xj = xy[j*2], yj = xy[j*2+1];
        if (((yi > py) != (yj > py)) &&
            (px < (long)(xj - xi) * (py - yi) / (yj - yi) + xi))
            inside = !inside;
    }
    return inside;
}

int nova_poly_area2(const int *xy, int n)
{
    long area = 0;
    for (int i = 0, j = n - 1; i < n; j = i++)
        area += (long)(xy[j*2] + xy[i*2]) * (xy[j*2+1] - xy[i*2+1]);
    if (area < 0) area = -area;
    return (int)area;
}

void nova_poly_fill(nova_machine *mc, const int *xy, int n, uint8_t color)
{
    if (n < 3) return;
    int ymin = xy[1], ymax = xy[1];
    for (int i = 1; i < n; i++) {
        if (xy[i*2+1] < ymin) ymin = xy[i*2+1];
        if (xy[i*2+1] > ymax) ymax = xy[i*2+1];
    }
    if (ymin < 0) ymin = 0;
    if (ymax >= NOVA_FB_H) ymax = NOVA_FB_H - 1;

    int xs[64];
    for (int y = ymin; y <= ymax; y++) {
        int nx = 0;
        for (int i = 0, j = n - 1; i < n; j = i++) {
            int yi = xy[i*2+1], yj = xy[j*2+1];
            if ((yi <= y && yj > y) || (yj <= y && yi > y)) {
                int xi = xy[i*2], xj = xy[j*2];
                int x = xi + (int)((long)(y - yi) * (xj - xi) / (yj - yi));
                if (nx < 64) xs[nx++] = x;
            }
        }
        /* sort intersections */
        for (int a = 1; a < nx; a++) {
            int key = xs[a], b = a - 1;
            while (b >= 0 && xs[b] > key) { xs[b+1] = xs[b]; b--; }
            xs[b+1] = key;
        }
        for (int a = 0; a + 1 < nx; a += 2)
            nova_raster_hline(mc, xs[a], xs[a+1], y, color);
    }
}
