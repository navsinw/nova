#include "nova.h"

/* 16.16 fixed-point 2D vector math for movement and steering. */

nova_vec2 nova_v2_add(nova_vec2 a, nova_vec2 b) { nova_vec2 r = { a.x + b.x, a.y + b.y }; return r; }
nova_vec2 nova_v2_sub(nova_vec2 a, nova_vec2 b) { nova_vec2 r = { a.x - b.x, a.y - b.y }; return r; }
nova_vec2 nova_v2_scale(nova_vec2 a, int32_t s) { nova_vec2 r = { nova_fp_mul(a.x, s), nova_fp_mul(a.y, s) }; return r; }

int32_t nova_v2_dot(nova_vec2 a, nova_vec2 b) { return nova_fp_mul(a.x, b.x) + nova_fp_mul(a.y, b.y); }
int32_t nova_v2_cross(nova_vec2 a, nova_vec2 b) { return nova_fp_mul(a.x, b.y) - nova_fp_mul(a.y, b.x); }

int32_t nova_v2_len(nova_vec2 a)
{
    int32_t d = nova_fp_mul(a.x, a.x) + nova_fp_mul(a.y, a.y);
    return nova_fp_sqrt(d);
}

nova_vec2 nova_v2_normalize(nova_vec2 a)
{
    int32_t l = nova_v2_len(a);
    nova_vec2 r = { 0, 0 };
    if (l <= 0) return r;
    r.x = nova_fp_div(a.x, l);
    r.y = nova_fp_div(a.y, l);
    return r;
}

nova_vec2 nova_v2_rotate(nova_vec2 a, int angle)
{
    int32_t c = nova_fp_cos(angle << 8);
    int32_t s = nova_fp_sin(angle << 8);
    nova_vec2 r;
    r.x = nova_fp_mul(a.x, c) - nova_fp_mul(a.y, s);
    r.y = nova_fp_mul(a.x, s) + nova_fp_mul(a.y, c);
    return r;
}
