#include "nova.h"

/* standard base64, used for embedding small binary blobs in text manifests. */

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int b64_val(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int nova_b64_encode(const uint8_t *in, int inlen, char *out, int outcap)
{
    int op = 0;
    for (int i = 0; i < inlen; i += 3) {
        int b0 = in[i];
        int b1 = (i + 1 < inlen) ? in[i + 1] : 0;
        int b2 = (i + 2 < inlen) ? in[i + 2] : 0;
        if (op + 4 > outcap - 1) break;
        out[op++] = B64[b0 >> 2];
        out[op++] = B64[((b0 & 3) << 4) | (b1 >> 4)];
        out[op++] = (i + 1 < inlen) ? B64[((b1 & 15) << 2) | (b2 >> 6)] : '=';
        out[op++] = (i + 2 < inlen) ? B64[b2 & 63] : '=';
    }
    out[op] = 0;
    return op;
}

int nova_b64_decode(const char *in, int inlen, uint8_t *out, int outcap)
{
    int op = 0, acc = 0, bits = 0;
    for (int i = 0; i < inlen; i++) {
        if (in[i] == '=' || in[i] == 0) break;
        int v = b64_val(in[i]);
        if (v < 0) continue;
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (op >= outcap) break;
            out[op++] = (uint8_t)((acc >> bits) & 0xff);
        }
    }
    return op;
}
