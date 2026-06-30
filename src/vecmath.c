#include "nova.h"

/* additional 2D vector operations layered on vec2.c. */

nova_vec2 nova_v2_lerp(nova_vec2 a, nova_vec2 b, int32_t t)
{
    nova_vec2 r;
    r.x = a.x + nova_fp_mul(b.x - a.x, t);
    r.y = a.y + nova_fp_mul(b.y - a.y, t);
    return r;
}

nova_vec2 nova_v2_perp(nova_vec2 a)
{
    nova_vec2 r = { -a.y, a.x };
    return r;
}

int32_t nova_v2_dist(nova_vec2 a, nova_vec2 b)
{
    return nova_v2_len(nova_v2_sub(a, b));
}

nova_vec2 nova_v2_project(nova_vec2 a, nova_vec2 onto)
{
    int32_t denom = nova_v2_dot(onto, onto);
    if (denom == 0) { nova_vec2 z = { 0, 0 }; return z; }
    int32_t scale = nova_fp_div(nova_v2_dot(a, onto), denom);
    return nova_v2_scale(onto, scale);
}

nova_vec2 nova_v2_reflect(nova_vec2 v, nova_vec2 n)
{
    nova_vec2 nn = nova_v2_normalize(n);
    int32_t d = nova_v2_dot(v, nn);
    nova_vec2 r;
    r.x = v.x - nova_fp_mul(2 * NOVA_FP_ONE, nova_fp_mul(d, nn.x));
    r.y = v.y - nova_fp_mul(2 * NOVA_FP_ONE, nova_fp_mul(d, nn.y));
    return r;
}
