#include "nova.h"

/* easing curves on a 16.16 parameter t in [0,1]; used for animation. */

static int32_t clamp01(int32_t t)
{
    if (t < 0) return 0;
    if (t > NOVA_FP_ONE) return NOVA_FP_ONE;
    return t;
}

int32_t nova_ease_linear(int32_t t) { return clamp01(t); }

int32_t nova_ease_in_quad(int32_t t)
{
    t = clamp01(t);
    return nova_fp_mul(t, t);
}

int32_t nova_ease_out_quad(int32_t t)
{
    t = clamp01(t);
    int32_t inv = NOVA_FP_ONE - t;
    return NOVA_FP_ONE - nova_fp_mul(inv, inv);
}

int32_t nova_ease_inout_quad(int32_t t)
{
    t = clamp01(t);
    if (t < NOVA_FP_ONE / 2)
        return 2 * nova_fp_mul(t, t);
    int32_t u = t - NOVA_FP_ONE;
    return NOVA_FP_ONE - 2 * nova_fp_mul(u, u);
}

int32_t nova_ease_out_bounce(int32_t t)
{
    t = clamp01(t);
    const int32_t n1 = (int32_t)(7.5625 * NOVA_FP_ONE);
    const int32_t d1 = (int32_t)(2.75 * NOVA_FP_ONE);
    if (t < nova_fp_div(NOVA_FP_ONE, d1)) {
        return nova_fp_mul(n1, nova_fp_mul(t, t));
    } else if (t < nova_fp_div(2 * NOVA_FP_ONE, d1)) {
        int32_t u = t - nova_fp_div((int32_t)(1.5 * NOVA_FP_ONE), d1);
        return nova_fp_mul(n1, nova_fp_mul(u, u)) + (int32_t)(0.75 * NOVA_FP_ONE);
    } else if (t < nova_fp_div((int32_t)(2.5 * NOVA_FP_ONE), d1)) {
        int32_t u = t - nova_fp_div((int32_t)(2.25 * NOVA_FP_ONE), d1);
        return nova_fp_mul(n1, nova_fp_mul(u, u)) + (int32_t)(0.9375 * NOVA_FP_ONE);
    } else {
        int32_t u = t - nova_fp_div((int32_t)(2.625 * NOVA_FP_ONE), d1);
        return nova_fp_mul(n1, nova_fp_mul(u, u)) + (int32_t)(0.984375 * NOVA_FP_ONE);
    }
}

int32_t nova_tween(int32_t a, int32_t b, int32_t t, int kind)
{
    int32_t e;
    switch (kind) {
    case 1: e = nova_ease_in_quad(t); break;
    case 2: e = nova_ease_out_quad(t); break;
    case 3: e = nova_ease_inout_quad(t); break;
    case 4: e = nova_ease_out_bounce(t); break;
    default: e = nova_ease_linear(t); break;
    }
    return a + nova_fp_mul(b - a, e);
}
