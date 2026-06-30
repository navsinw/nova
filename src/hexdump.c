#include "nova.h"
#include <stdio.h>

/* classic 16-byte-per-row hex + ASCII dump into a caller buffer. */

int nova_hexdump(const uint8_t *data, int len, char *out, int outcap)
{
    int op = 0;
    for (int off = 0; off < len; off += 16) {
        int n = snprintf(out + op, outcap - op, "%08x  ", off);
        if (n < 0 || op + n >= outcap) break;
        op += n;
        for (int i = 0; i < 16; i++) {
            if (off + i < len)
                n = snprintf(out + op, outcap - op, "%02x ", data[off + i]);
            else
                n = snprintf(out + op, outcap - op, "   ");
            if (n < 0 || op + n >= outcap) return op;
            op += n;
            if (i == 7) { if (op < outcap - 1) out[op++] = ' '; }
        }
        if (op < outcap - 1) out[op++] = '|';
        for (int i = 0; i < 16 && off + i < len; i++) {
            uint8_t c = data[off + i];
            if (op >= outcap - 1) return op;
            out[op++] = (c >= 32 && c < 127) ? (char)c : '.';
        }
        if (op < outcap - 1) out[op++] = '|';
        if (op < outcap - 1) out[op++] = '\n';
    }
    if (op < outcap) out[op] = 0;
    return op;
}
