#include "nova.h"
#include <string.h>

/* canonical Huffman coding. layout: [u32 origlen][256 code-length bytes]
   [MSB-first bitstream]. lengths are derived from a textbook two-smallest
   merge; codes are assigned canonically so the decoder only needs lengths. */

static void build_lengths(const uint8_t *in, int inlen, uint8_t lens[256])
{
    long freq[256];
    for (int i = 0; i < 256; i++) freq[i] = 0;
    for (int i = 0; i < inlen; i++) freq[in[i]]++;

    /* node pool: leaves 0..255, internal nodes appended */
    long w[512];
    int left[512], right[512], parent[512];
    int nnodes = 0;
    int leaf[256];
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) { w[nnodes] = freq[i]; left[nnodes] = right[nnodes] = -1; leaf[i] = nnodes; nnodes++; }
        else leaf[i] = -1;
    }
    for (int i = 0; i < 256; i++) lens[i] = 0;
    if (nnodes == 0) return;
    if (nnodes == 1) { for (int i = 0; i < 256; i++) if (leaf[i] >= 0) lens[i] = 1; return; }

    int used[512];
    for (int i = 0; i < nnodes; i++) used[i] = 0;
    int remaining = nnodes;
    while (remaining > 1) {
        int a = -1, b = -1;
        for (int i = 0; i < nnodes; i++) {
            if (used[i]) continue;
            if (a < 0 || w[i] < w[a]) { b = a; a = i; }
            else if (b < 0 || w[i] < w[b]) b = i;
        }
        int p = nnodes++;
        w[p] = w[a] + w[b];
        left[p] = a; right[p] = b; parent[a] = p; parent[b] = p;
        used[a] = used[b] = 1; used[p] = 0;
        remaining--;
    }
    int root = nnodes - 1;
    for (int i = 0; i < 256; i++) {
        if (leaf[i] < 0) continue;
        int d = 0, cur = leaf[i];
        while (cur != root && d < 255) { cur = parent[cur]; d++; }
        lens[i] = (uint8_t)(d ? d : 1);
    }
}

static void canonical(const uint8_t lens[256], uint32_t codes[256])
{
    int bl_count[256];
    memset(bl_count, 0, sizeof(bl_count));
    int maxlen = 0;
    for (int i = 0; i < 256; i++) { if (lens[i]) { bl_count[lens[i]]++; if (lens[i] > maxlen) maxlen = lens[i]; } }
    uint32_t next_code[256];
    memset(next_code, 0, sizeof(next_code));
    uint32_t code = 0;
    for (int L = 1; L <= maxlen; L++) { code = (code + (uint32_t)bl_count[L - 1]) << 1; next_code[L] = code; }
    for (int i = 0; i < 256; i++) if (lens[i]) codes[i] = next_code[lens[i]]++;
}

int nova_huff_encode(const uint8_t *in, int inlen, uint8_t *out, int outcap)
{
    if (inlen < 0) return -1;
    if (outcap < 4 + 256) return -1;
    uint8_t lens[256];
    uint32_t codes[256];
    build_lengths(in, inlen, lens);
    canonical(lens, codes);

    out[0] = (uint8_t)(inlen & 0xff); out[1] = (uint8_t)((inlen >> 8) & 0xff);
    out[2] = (uint8_t)((inlen >> 16) & 0xff); out[3] = (uint8_t)((inlen >> 24) & 0xff);
    memcpy(out + 4, lens, 256);

    int op = 4 + 256;
    int bitpos = 0;
    out[op] = 0;
    for (int i = 0; i < inlen; i++) {
        uint8_t sym = in[i];
        int L = lens[sym];
        uint32_t c = codes[sym];
        for (int b = L - 1; b >= 0; b--) {
            if (op >= outcap) return -1;
            if ((c >> b) & 1) out[op] |= (uint8_t)(0x80 >> bitpos);
            bitpos++;
            if (bitpos == 8) { bitpos = 0; op++; if (op < outcap) out[op] = 0; }
        }
    }
    if (bitpos > 0) op++;
    return op;
}

int nova_huff_decode(const uint8_t *in, int inlen, uint8_t *out, int outcap)
{
    if (inlen < 4 + 256) return -1;
    uint32_t origlen = (uint32_t)in[0] | ((uint32_t)in[1]<<8) | ((uint32_t)in[2]<<16) | ((uint32_t)in[3]<<24);
    const uint8_t *lens = in + 4;

    int bl_count[256];
    memset(bl_count, 0, sizeof(bl_count));
    int maxlen = 0;
    for (int i = 0; i < 256; i++) { if (lens[i]) { bl_count[lens[i]]++; if (lens[i] > maxlen) maxlen = lens[i]; } }

    uint32_t first_code[256];
    int first_index[256];
    memset(first_code, 0, sizeof(first_code));
    memset(first_index, 0, sizeof(first_index));
    uint32_t code = 0;
    int idx = 0;
    int order[256];
    for (int L = 1; L <= maxlen; L++) {
        code = (code + (uint32_t)bl_count[L - 1]) << 1;
        first_code[L] = code;
        first_index[L] = idx;
        for (int s = 0; s < 256; s++) if (lens[s] == L) order[idx++] = s;
    }

    int ip = 4 + 256;
    int bitpos = 0;
    uint32_t cur = 0;
    int curlen = 0;
    uint32_t produced = 0;
    while (produced < origlen) {
        if (ip >= inlen) break;
        int bit = (in[ip] >> (7 - bitpos)) & 1;
        bitpos++;
        if (bitpos == 8) { bitpos = 0; ip++; }
        cur = (cur << 1) | (uint32_t)bit;
        curlen++;
        if (curlen <= maxlen && bl_count[curlen] > 0) {
            uint32_t fc = first_code[curlen];
            if (cur >= fc && cur - fc < (uint32_t)bl_count[curlen]) {
                int sym = order[first_index[curlen] + (int)(cur - fc)];
                if ((int)produced >= outcap) break;
                out[produced++] = (uint8_t)sym;
                cur = 0; curlen = 0;
            }
        }
        if (curlen > maxlen) break;
    }
    return (int)produced;
}
