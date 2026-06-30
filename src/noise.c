#include "nova.h"

/* value noise with a permutation table; fbm sums octaves. outputs 16.16. */

void nova_noise_init(nova_noise *n, uint32_t seed)
{
    nova_rng r;
    nova_rng_seed(&r, seed);
    for (int i = 0; i < 256; i++) n->perm[i] = (uint8_t)i;
    for (int i = 255; i > 0; i--) {
        int j = (int)(nova_rng_next(&r) % (uint32_t)(i + 1));
        uint8_t t = n->perm[i]; n->perm[i] = n->perm[j]; n->perm[j] = t;
    }
    for (int i = 0; i < 256; i++) n->perm[256 + i] = n->perm[i];
}

static int32_t grad_val(nova_noise *n, int xi, int yi)
{
    int h = n->perm[(n->perm[xi & 255] + yi) & 511];
    return ((int32_t)h - 128) << 9;   /* roughly [-1,1] in 16.16 */
}

static int32_t smooth(int32_t t)
{
    /* 3t^2 - 2t^3 in 16.16 */
    int32_t t2 = nova_fp_mul(t, t);
    int32_t t3 = nova_fp_mul(t2, t);
    return 3 * t2 - 2 * t3;
}

int32_t nova_noise_value(nova_noise *n, int32_t x, int32_t y)
{
    int xi = x >> 16, yi = y >> 16;
    int32_t xf = x & 0xffff, yf = y & 0xffff;
    int32_t v00 = grad_val(n, xi, yi);
    int32_t v10 = grad_val(n, xi + 1, yi);
    int32_t v01 = grad_val(n, xi, yi + 1);
    int32_t v11 = grad_val(n, xi + 1, yi + 1);
    int32_t sx = smooth(xf);
    int32_t sy = smooth(yf);
    int32_t a = v00 + nova_fp_mul(sx, v10 - v00);
    int32_t b = v01 + nova_fp_mul(sx, v11 - v01);
    return a + nova_fp_mul(sy, b - a);
}

int32_t nova_noise_fbm(nova_noise *n, int32_t x, int32_t y, int octaves)
{
    if (octaves < 1) octaves = 1;
    if (octaves > 8) octaves = 8;
    int32_t sum = 0, amp = NOVA_FP_ONE, freq = NOVA_FP_ONE;
    for (int o = 0; o < octaves; o++) {
        int32_t nx = nova_fp_mul(x, freq);
        int32_t ny = nova_fp_mul(y, freq);
        sum += nova_fp_mul(nova_noise_value(n, nx, ny), amp);
        amp >>= 1;
        freq <<= 1;
    }
    return sum;
}
