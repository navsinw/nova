#include "nova.h"

/* 16.16 fixed-point helpers for the console's geometry and audio code. */

static int32_t sin_tab[256];
static int     sin_ready;

static void build_sin(void)
{
    if (sin_ready) return;
    /* quarter-wave parabola pieced into a full period; good enough for an 8-bit toy */
    for (int i = 0; i < 256; i++) {
        int q = i & 0x3f;
        int seg = (i >> 6) & 3;
        int32_t v;
        int t = q * 1024;          /* 0..64k */
        int32_t base = (t * (65536 - t)) >> 14;
        if (base > 65536) base = 65536;
        switch (seg) {
        case 0: v = base; break;
        case 1: v = 65536 - base; break;
        case 2: v = -base; break;
        default: v = -(65536 - base); break;
        }
        sin_tab[i] = v;
    }
    sin_ready = 1;
}

int32_t nova_fp_mul(int32_t a, int32_t b)
{
    int64_t r = (int64_t)a * (int64_t)b;
    return (int32_t)(r >> 16);
}

int32_t nova_fp_div(int32_t a, int32_t b)
{
    if (b == 0) return a >= 0 ? 0x7fffffff : (int32_t)0x80000000;
    int64_t r = ((int64_t)a << 16) / b;
    return (int32_t)r;
}

int32_t nova_fp_sin(int32_t ang)
{
    build_sin();
    uint32_t idx = ((uint32_t)ang >> 8) & 0xff;
    return sin_tab[idx];
}

int32_t nova_fp_cos(int32_t ang)
{
    return nova_fp_sin(ang + (64 << 8));
}

int32_t nova_fp_sqrt(int32_t v)
{
    if (v <= 0) return 0;
    int64_t x = (int64_t)v << 16;
    int64_t r = x;
    int64_t last = 0;
    for (int i = 0; i < 32 && r != last; i++) {
        last = r;
        if (r == 0) break;
        r = (r + x / r) / 2;
    }
    return (int32_t)r;
}

int nova_lerp(int a, int b, int t, int tmax)
{
    if (tmax <= 0) return a;
    if (t < 0) t = 0;
    if (t > tmax) t = tmax;
    return a + (b - a) * t / tmax;
}

int nova_clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
