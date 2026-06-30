#include "nova.h"
#include <stdio.h>
#include <stdlib.h>

/* nova_dis: disassemble the CODE chunk of a .nova cartridge. */

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
    *len = fread(buf, 1, (size_t)n, f);
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s <cart.nova>\n", argv[0]); return 2; }
    size_t len = 0;
    uint8_t *data = read_file(argv[1], &len);
    if (!data) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }

    nova_cart cart;
    if (nova_cart_open(&cart, data, len) != 0) {
        fprintf(stderr, "invalid cartridge\n"); free(data); return 1;
    }
    const uint8_t *code; uint32_t clen;
    if (nova_cart_find(&cart, TAG_CODE, &code, &clen) != 0) {
        fprintf(stderr, "no CODE chunk\n"); nova_cart_close(&cart); free(data); return 1;
    }
    if ((size_t)(code - data) + clen > len) clen = (uint32_t)(len - (size_t)(code - data));

    char line[96];
    uint32_t pc = 0;
    while (pc < clen) {
        int adv = nova_disasm_one(code, clen, pc, line, sizeof(line));
        if (adv <= 0) break;
        printf("%s\n", line);
        pc += (uint32_t)adv;
    }
    nova_cart_close(&cart);
    free(data);
    return 0;
}
