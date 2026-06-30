#include "nova.h"

/* a few more non-cryptographic hashes for hash-table experiments. */

uint32_t nova_djb2(const uint8_t *p, size_t n)
{
    uint32_t h = 5381;
    for (size_t i = 0; i < n; i++) h = ((h << 5) + h) + p[i];
    return h;
}

uint32_t nova_sdbm(const uint8_t *p, size_t n)
{
    uint32_t h = 0;
    for (size_t i = 0; i < n; i++) h = p[i] + (h << 6) + (h << 16) - h;
    return h;
}

static uint32_t rotl32(uint32_t x, int r) { return (x << r) | (x >> (32 - r)); }

uint32_t nova_murmur3(const uint8_t *p, size_t n, uint32_t seed)
{
    uint32_t h = seed;
    const uint32_t c1 = 0xcc9e2d51u, c2 = 0x1b873593u;
    size_t nblocks = n / 4;
    for (size_t i = 0; i < nblocks; i++) {
        uint32_t k = (uint32_t)p[i*4] | ((uint32_t)p[i*4+1] << 8) |
                     ((uint32_t)p[i*4+2] << 16) | ((uint32_t)p[i*4+3] << 24);
        k *= c1; k = rotl32(k, 15); k *= c2;
        h ^= k; h = rotl32(h, 13); h = h * 5 + 0xe6546b64u;
    }
    uint32_t k = 0;
    const uint8_t *tail = p + nblocks * 4;
    switch (n & 3) {
    case 3: k ^= (uint32_t)tail[2] << 16; /* fall through */
    case 2: k ^= (uint32_t)tail[1] << 8;  /* fall through */
    case 1: k ^= (uint32_t)tail[0];
            k *= c1; k = rotl32(k, 15); k *= c2; h ^= k;
    }
    h ^= (uint32_t)n;
    h ^= h >> 16; h *= 0x85ebca6bu; h ^= h >> 13; h *= 0xc2b2ae35u; h ^= h >> 16;
    return h;
}
