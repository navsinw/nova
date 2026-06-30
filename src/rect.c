#include "nova.h"

/* axis-aligned rectangle helpers. */

int nova_rect_contains(nova_rect r, int x, int y)
{
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

int nova_rect_intersect(nova_rect a, nova_rect b, nova_rect *out)
{
    int x0 = a.x > b.x ? a.x : b.x;
    int y0 = a.y > b.y ? a.y : b.y;
    int x1 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    int y1 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);
    if (x1 <= x0 || y1 <= y0) {
        if (out) { out->x = out->y = out->w = out->h = 0; }
        return 0;
    }
    if (out) { out->x = x0; out->y = y0; out->w = x1 - x0; out->h = y1 - y0; }
    return 1;
}

nova_rect nova_rect_union(nova_rect a, nova_rect b)
{
    int x0 = a.x < b.x ? a.x : b.x;
    int y0 = a.y < b.y ? a.y : b.y;
    int x1 = (a.x + a.w) > (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    int y1 = (a.y + a.h) > (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);
    nova_rect r = { x0, y0, x1 - x0, y1 - y0 };
    return r;
}

nova_rect nova_rect_clamp(nova_rect r, nova_rect bounds)
{
    if (r.x < bounds.x) r.x = bounds.x;
    if (r.y < bounds.y) r.y = bounds.y;
    if (r.x + r.w > bounds.x + bounds.w) r.w = bounds.x + bounds.w - r.x;
    if (r.y + r.h > bounds.y + bounds.h) r.h = bounds.y + bounds.h - r.y;
    if (r.w < 0) r.w = 0;
    if (r.h < 0) r.h = 0;
    return r;
}
