#include "nova.h"

/* Bezier evaluation in 16.16 and a flattened polyline draw. */

nova_vec2 nova_bezier_quad(nova_vec2 a, nova_vec2 b, nova_vec2 c, int32_t t)
{
    int32_t u = NOVA_FP_ONE - t;
    int32_t uu = nova_fp_mul(u, u);
    int32_t tt = nova_fp_mul(t, t);
    int32_t ut2 = nova_fp_mul(nova_fp_mul(u, t), 2 * NOVA_FP_ONE);
    nova_vec2 r;
    r.x = nova_fp_mul(uu, a.x) + nova_fp_mul(ut2, b.x) + nova_fp_mul(tt, c.x);
    r.y = nova_fp_mul(uu, a.y) + nova_fp_mul(ut2, b.y) + nova_fp_mul(tt, c.y);
    return r;
}

nova_vec2 nova_bezier_cubic(nova_vec2 a, nova_vec2 b, nova_vec2 c, nova_vec2 d, int32_t t)
{
    int32_t u = NOVA_FP_ONE - t;
    int32_t uu = nova_fp_mul(u, u), uuu = nova_fp_mul(uu, u);
    int32_t tt = nova_fp_mul(t, t), ttt = nova_fp_mul(tt, t);
    int32_t c1 = nova_fp_mul(3 * NOVA_FP_ONE, nova_fp_mul(uu, t));
    int32_t c2 = nova_fp_mul(3 * NOVA_FP_ONE, nova_fp_mul(u, tt));
    nova_vec2 r;
    r.x = nova_fp_mul(uuu, a.x) + nova_fp_mul(c1, b.x) + nova_fp_mul(c2, c.x) + nova_fp_mul(ttt, d.x);
    r.y = nova_fp_mul(uuu, a.y) + nova_fp_mul(c1, b.y) + nova_fp_mul(c2, c.y) + nova_fp_mul(ttt, d.y);
    return r;
}

void nova_bezier_draw(nova_machine *mc, nova_vec2 a, nova_vec2 b, nova_vec2 c, nova_vec2 d, int segs, uint8_t color)
{
    if (segs < 1) segs = 1;
    if (segs > 256) segs = 256;
    nova_vec2 prev = a;
    for (int i = 1; i <= segs; i++) {
        int32_t t = (int32_t)((int64_t)i * NOVA_FP_ONE / segs);
        nova_vec2 p = nova_bezier_cubic(a, b, c, d, t);
        nova_raster_line(mc, prev.x >> 16, prev.y >> 16, p.x >> 16, p.y >> 16, color);
        prev = p;
    }
}
