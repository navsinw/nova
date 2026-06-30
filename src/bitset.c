#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* fixed-size bitset with population count. */

int nova_bitset_init(nova_bitset *b, int nbits)
{
    if (nbits < 1) nbits = 1;
    int words = (nbits + 31) / 32;
    b->words = (uint32_t*)calloc((size_t)words, sizeof(uint32_t));
    if (!b->words) return -1;
    b->nbits = nbits;
    return 0;
}

void nova_bitset_set(nova_bitset *b, int i)
{
    if (i < 0 || i >= b->nbits) return;
    b->words[i >> 5] |= (1u << (i & 31));
}

void nova_bitset_clear(nova_bitset *b, int i)
{
    if (i < 0 || i >= b->nbits) return;
    b->words[i >> 5] &= ~(1u << (i & 31));
}

int nova_bitset_test(nova_bitset *b, int i)
{
    if (i < 0 || i >= b->nbits) return 0;
    return (b->words[i >> 5] >> (i & 31)) & 1u;
}

int nova_bitset_count(nova_bitset *b)
{
    int words = (b->nbits + 31) / 32;
    int total = 0;
    for (int w = 0; w < words; w++) {
        uint32_t x = b->words[w];
        while (x) { x &= x - 1; total++; }
    }
    return total;
}

void nova_bitset_free(nova_bitset *b)
{
    free(b->words);
    b->words = NULL;
    b->nbits = 0;
}
