#include "nova.h"
#include <stdio.h>
#include <string.h>

/* a static cartridge linter: opens a cart and reports structural issues
   without executing it. read-only; never mutates the input. */

static int append(char *out, int cap, int *op, const char *msg)
{
    int n = (int)strlen(msg);
    if (*op + n + 1 >= cap) return 0;
    memcpy(out + *op, msg, n);
    *op += n;
    out[*op] = 0;
    return 1;
}

int nova_validate_cart(const uint8_t *data, size_t len, char *report, int cap)
{
    int issues = 0;
    int op = 0;
    if (report && cap > 0) report[0] = 0;

    nova_cart c;
    if (nova_cart_open(&c, data, len) != 0) {
        if (report) append(report, cap, &op, "invalid or unreadable cartridge\n");
        return 1;
    }

    int has_code = 0;
    char line[96];
    for (int i = 0; i < c.chunk_cnt; i++) {
        if (c.dir[i].tag == TAG_CODE) has_code = 1;
        if (c.dir[i].size == 0) {
            issues++;
            if (report) { snprintf(line, sizeof(line), "chunk %d is empty\n", i); append(report, cap, &op, line); }
        }
        if ((size_t)c.dir[i].offset + c.dir[i].size > len) {
            issues++;
            if (report) { snprintf(line, sizeof(line), "chunk %d extends past file\n", i); append(report, cap, &op, line); }
        }
    }
    if (!has_code) {
        issues++;
        if (report) append(report, cap, &op, "no CODE chunk\n");
    }
    if (c.entry_pc != 0) {
        const uint8_t *code; uint32_t clen;
        if (nova_cart_find(&c, TAG_CODE, &code, &clen) == 0 && c.entry_pc >= clen) {
            issues++;
            if (report) append(report, cap, &op, "entry point outside CODE\n");
        }
    }

    nova_cart_close(&c);
    return issues;
}
