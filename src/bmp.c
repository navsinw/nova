#include "nova.h"
#include <stdio.h>

/* encode the index framebuffer as an ASCII PGM (P2) into a caller buffer.
   palette indices are mapped to a coarse grey ramp. */

int nova_fb_to_pgm(const uint8_t *fb, int w, int h, char *out, int outcap)
{
    if (!fb || w <= 0 || h <= 0) return 0;
    int op = 0;
    int n = snprintf(out + op, outcap - op, "P2\n%d %d\n255\n", w, h);
    if (n < 0 || op + n >= outcap) return op;
    op += n;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int v = fb[y * w + x];
            int grey = (v * 17) & 0xff;
            n = snprintf(out + op, outcap - op, "%d ", grey);
            if (n < 0 || op + n >= outcap) return op;
            op += n;
        }
        if (op < outcap - 1) out[op++] = '\n';
    }
    if (op < outcap) out[op] = 0;
    return op;
}
