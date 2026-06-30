#include "nova.h"

/* LZ77 encoder producing the token stream consumed by nova_lz_decode():
   a flag byte gates 8 items; a literal is one raw byte, a back-reference is a
   little-endian u16 holding offset(12) and length(4). */

#define LZ_WINDOW 4096
#define LZ_MINLEN 3
#define LZ_MAXLEN 18

int nova_lz_encode(const uint8_t *in, int inlen, uint8_t *out, int outcap)
{
    int ip = 0, op = 0;
    while (ip < inlen) {
        if (op >= outcap) break;
        int flagpos = op++;
        uint8_t flags = 0;

        for (int b = 0; b < 8 && ip < inlen; b++) {
            int best_len = 0, best_dist = 0;
            int start = ip - LZ_WINDOW;
            if (start < 0) start = 0;
            for (int j = start; j < ip; j++) {
                int len = 0;
                while (len < LZ_MAXLEN && ip + len < inlen && in[j + len] == in[ip + len])
                    len++;
                if (len > best_len) { best_len = len; best_dist = ip - j; }
            }
            if (best_len >= LZ_MINLEN) {
                if (op + 2 > outcap) { ip = inlen; break; }
                int tok = ((best_dist - 1) << 4) | (best_len - LZ_MINLEN);
                out[op++] = (uint8_t)(tok & 0xff);
                out[op++] = (uint8_t)((tok >> 8) & 0xff);
                ip += best_len;
                /* flag bit stays 0 for back-reference */
            } else {
                if (op >= outcap) { ip = inlen; break; }
                flags |= (uint8_t)(1 << b);
                out[op++] = in[ip++];
            }
        }
        out[flagpos] = flags;
    }
    return op;
}
