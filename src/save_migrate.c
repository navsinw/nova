#include "nova.h"
#include <string.h>

/* versioned save-blob migration. each version bumped the record layout; this
   walks old formats forward into the current one. all copies are clamped. */

int nova_save_version(const uint8_t *in, int inlen)
{
    if (!in || inlen < 4) return -1;
    if (in[0] != 'N' || in[1] != 'S') return -1;
    return in[2];
}

static int put(uint8_t *out, int outcap, int *op, const uint8_t *src, int n)
{
    if (*op + n > outcap) return -1;
    memcpy(out + *op, src, (size_t)n);
    *op += n;
    return 0;
}

int nova_save_migrate(const uint8_t *in, int inlen, uint8_t *out, int outcap)
{
    int ver = nova_save_version(in, inlen);
    if (ver < 1 || ver > 3) return -1;
    if (outcap < 8) return -1;

    int op = 0;
    uint8_t hdr[4] = { 'N', 'S', 3, 0 };
    if (put(out, outcap, &op, hdr, 4) != 0) return -1;

    int ip = 4;
    /* v1: records are [u8 tag][u8 val]; v2: [u8 tag][u16 val]; v3: [u8 tag][u32 val] */
    while (ip < inlen) {
        uint8_t tag = in[ip++];
        uint32_t val = 0;
        if (ver == 1) {
            if (ip + 1 > inlen) break;
            val = in[ip]; ip += 1;
        } else if (ver == 2) {
            if (ip + 2 > inlen) break;
            val = (uint32_t)(in[ip] | (in[ip+1] << 8)); ip += 2;
        } else {
            if (ip + 4 > inlen) break;
            val = (uint32_t)in[ip] | ((uint32_t)in[ip+1]<<8) | ((uint32_t)in[ip+2]<<16) | ((uint32_t)in[ip+3]<<24);
            ip += 4;
        }
        uint8_t rec[5];
        rec[0] = tag;
        rec[1] = val & 0xff; rec[2] = (val>>8)&0xff; rec[3] = (val>>16)&0xff; rec[4] = (val>>24)&0xff;
        if (put(out, outcap, &op, rec, 5) != 0) break;
    }
    return op;
}
