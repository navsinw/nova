#include "nova.h"
#include <string.h>

/* fixed 128-bit unsigned integer for high-score accumulation and hashing. */

void nova_u128_set(nova_u128 *a, uint32_t v)
{
    a->w[0] = v; a->w[1] = a->w[2] = a->w[3] = 0;
}

void nova_u128_add(nova_u128 *a, const nova_u128 *b)
{
    uint64_t carry = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t sum = (uint64_t)a->w[i] + b->w[i] + carry;
        a->w[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
}

void nova_u128_mul_u32(nova_u128 *a, uint32_t b)
{
    uint64_t carry = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t prod = (uint64_t)a->w[i] * b + carry;
        a->w[i] = (uint32_t)prod;
        carry = prod >> 32;
    }
}

int nova_u128_cmp(const nova_u128 *a, const nova_u128 *b)
{
    for (int i = 3; i >= 0; i--) {
        if (a->w[i] < b->w[i]) return -1;
        if (a->w[i] > b->w[i]) return 1;
    }
    return 0;
}

static int is_zero(const nova_u128 *a)
{
    return (a->w[0] | a->w[1] | a->w[2] | a->w[3]) == 0;
}

static uint32_t divmod10(nova_u128 *a)
{
    uint64_t rem = 0;
    for (int i = 3; i >= 0; i--) {
        uint64_t cur = (rem << 32) | a->w[i];
        a->w[i] = (uint32_t)(cur / 10);
        rem = cur % 10;
    }
    return (uint32_t)rem;
}

int nova_u128_to_dec(const nova_u128 *a, char *out, int outcap)
{
    if (outcap < 2) return 0;
    nova_u128 t = *a;
    char tmp[48];
    int n = 0;
    if (is_zero(&t)) { out[0] = '0'; out[1] = 0; return 1; }
    while (!is_zero(&t) && n < 47) {
        tmp[n++] = (char)('0' + divmod10(&t));
    }
    int op = 0;
    for (int i = n - 1; i >= 0 && op < outcap - 1; i--)
        out[op++] = tmp[i];
    out[op] = 0;
    return op;
}
