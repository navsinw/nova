#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* nova_info: print the chunk directory and integrity hashes of a .nova file. */

static void fourcc_str(uint32_t tag, char *out)
{
    out[0] = (char)(tag & 0xff);
    out[1] = (char)((tag >> 8) & 0xff);
    out[2] = (char)((tag >> 16) & 0xff);
    out[3] = (char)((tag >> 24) & 0xff);
    out[4] = 0;
    for (int i = 0; i < 4; i++)
        if (out[i] < 32 || out[i] > 126) out[i] = '.';
}

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t*)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    *len = got;
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <cart.nova>\n", argv[0]);
        return 2;
    }
    size_t len = 0;
    uint8_t *data = read_file(argv[1], &len);
    if (!data) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }

    printf("file:    %s\n", argv[1]);
    printf("size:    %zu bytes\n", len);
    printf("crc32:   %08x\n", nova_crc32(data, len));
    printf("adler32: %08x\n", nova_adler32(data, len));
    printf("fnv1a:   %08x\n", nova_fnv1a(data, len));

    nova_cart cart;
    if (nova_cart_open(&cart, data, len) != 0) {
        printf("not a valid NOVA-8 cartridge\n");
        free(data);
        return 1;
    }
    printf("version: %u  flags: 0x%04x  entry: 0x%x\n", cart.version, cart.flags, cart.entry_pc);
    printf("chunks:  %u\n", cart.chunk_cnt);
    char fc[5];
    for (int i = 0; i < cart.chunk_cnt; i++) {
        fourcc_str(cart.dir[i].tag, fc);
        printf("  [%2d] %-4s  off=%-8u size=%-8u\n", i, fc, cart.dir[i].offset, cart.dir[i].size);
    }
    nova_cart_close(&cart);
    free(data);
    return 0;
}
