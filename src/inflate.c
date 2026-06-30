#include "nova.h"

/* small bounded decompressors for optional packed cartridge chunks.
   both writers clamp to outcap and refuse malformed back-references. */

int nova_rle_decode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap)
{
    size_t ip = 0, op = 0;
    while (ip < inlen) {
        uint8_t ctrl = in[ip++];
        if (ctrl & 0x80) {
            int run = (ctrl & 0x7f) + 1;
            if (ip >= inlen) break;
            uint8_t val = in[ip++];
            for (int k = 0; k < run; k++) {
                if (op >= outcap) return (int)op;
                out[op++] = val;
            }
        } else {
            int lit = ctrl + 1;
            for (int k = 0; k < lit; k++) {
                if (ip >= inlen || op >= outcap) return (int)op;
                out[op++] = in[ip++];
            }
        }
    }
    return (int)op;
}

int nova_lz_decode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap)
{
    size_t ip = 0, op = 0;
    while (ip < inlen) {
        uint8_t flags = in[ip++];
        for (int b = 0; b < 8 && ip < inlen; b++) {
            if (flags & (1 << b)) {
                /* literal */
                if (op >= outcap) return (int)op;
                out[op++] = in[ip++];
            } else {
                /* back-reference: 2 bytes -> offset(12) + len(4) */
                if (ip + 1 >= inlen) return (int)op;
                uint16_t tok = (uint16_t)(in[ip] | (in[ip+1] << 8));
                ip += 2;
                int dist = (tok >> 4) + 1;
                int len = (tok & 0x0f) + 3;
                if ((size_t)dist > op) return (int)op;       /* refuse underflow */
                size_t src = op - (size_t)dist;
                for (int k = 0; k < len; k++) {
                    if (op >= outcap) return (int)op;
                    out[op] = out[src + k];
                    op++;
                }
            }
        }
    }
    return (int)op;
}
