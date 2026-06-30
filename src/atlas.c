#include "nova.h"

/* sprite-atlas codec: a byte-oriented RLE tuned for index images with long
   runs of the transparent colour 0. pack and unpack are inverses. */

int nova_atlas_pack(const uint8_t *pixels, int w, int h, uint8_t *out, int outcap)
{
    if (!pixels || w <= 0 || h <= 0) return 0;
    long total = (long)w * h;
    int op = 0;
    long i = 0;
    /* header: w,h as u16 each */
    if (outcap < 4) return 0;
    out[op++] = (uint8_t)(w & 0xff); out[op++] = (uint8_t)((w >> 8) & 0xff);
    out[op++] = (uint8_t)(h & 0xff); out[op++] = (uint8_t)((h >> 8) & 0xff);

    while (i < total) {
        uint8_t v = pixels[i];
        long run = 1;
        while (i + run < total && pixels[i + run] == v && run < 255) run++;
        if (op + 2 > outcap) break;
        out[op++] = (uint8_t)run;
        out[op++] = v;
        i += run;
    }
    return op;
}

int nova_atlas_unpack(const uint8_t *in, int inlen, uint8_t *out, int outcap)
{
    if (!in || inlen < 4) return 0;
    int ip = 4; /* skip w,h header */
    int op = 0;
    while (ip + 1 < inlen) {
        int run = in[ip++];
        uint8_t v = in[ip++];
        for (int k = 0; k < run; k++) {
            if (op >= outcap) return op;
            out[op++] = v;
        }
    }
    return op;
}
