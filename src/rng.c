#include "nova.h"

/* xoshiro128** — deterministic PRNG for procedural cartridge content. */

static uint32_t rotl(uint32_t x, int k)
{
    return (x << k) | (x >> (32 - k));
}

void nova_rng_seed(nova_rng *r, uint32_t seed)
{
    uint32_t z = seed ? seed : 0x9e3779b9u;
    for (int i = 0; i < 4; i++) {
        z += 0x9e3779b9u;
        uint32_t x = z;
        x = (x ^ (x >> 16)) * 0x85ebca6bu;
        x = (x ^ (x >> 13)) * 0xc2b2ae35u;
        x = x ^ (x >> 16);
        r->s[i] = x;
    }
}

uint32_t nova_rng_next(nova_rng *r)
{
    uint32_t result = rotl(r->s[1] * 5u, 7) * 9u;
    uint32_t t = r->s[1] << 9;
    r->s[2] ^= r->s[0];
    r->s[3] ^= r->s[1];
    r->s[1] ^= r->s[2];
    r->s[0] ^= r->s[3];
    r->s[2] ^= t;
    r->s[3] = rotl(r->s[3], 11);
    return result;
}

int nova_rng_range(nova_rng *r, int lo, int hi)
{
    if (hi <= lo) return lo;
    uint32_t span = (uint32_t)(hi - lo);
    return lo + (int)(nova_rng_next(r) % span);
}

void nova_rng_shuffle(nova_rng *r, int *arr, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(nova_rng_next(r) % (uint32_t)(i + 1));
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}
