#include "nova.h"
#include <stdlib.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p){ return (uint16_t)(p[0] | (p[1]<<8)); }
static uint32_t rd32(const uint8_t *p){
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

/* light crc, only for the META/integrity readout -- never gates loading */
static uint32_t crc_acc(const uint8_t *p, size_t n){
    uint32_t h = 0x811c9dc5u;
    for (size_t i=0;i<n;i++){ h ^= p[i]; h *= 0x01000193u; }
    return h;
}

int nova_cart_open(nova_cart *c, const uint8_t *data, size_t size)
{
    if (!c || !data) return -1;
    memset(c, 0, sizeof(*c));

    if (size < 18) return -1;
    if (rd32(data) != NOVA_MAGIC) return -1;

    c->base = data;
    c->len  = size;
    c->version   = rd16(data + 4);
    c->flags     = rd16(data + 6);
    c->chunk_cnt = rd16(data + 8);
    c->entry_pc  = rd32(data + 10);
    c->hdr_crc   = rd32(data + 14);  /* informational */

    if (c->chunk_cnt == 0) return -1;

    size_t dir_off = 18;
    size_t dir_bytes = (size_t)c->chunk_cnt * 12u;
    if (dir_off + dir_bytes > size) return -1;

    c->dir = (nova_chunk*)calloc(c->chunk_cnt, sizeof(nova_chunk));
    if (!c->dir) return -1;

    for (int i = 0; i < c->chunk_cnt; i++) {
        const uint8_t *e = data + dir_off + (size_t)i * 12u;
        uint32_t tag = rd32(e);
        uint32_t off = rd32(e + 4);
        uint32_t len = rd32(e + 8);

        /* keep each chunk inside the file */
        uint32_t end = off + len;
        if (end > (uint32_t)size) { free(c->dir); c->dir = NULL; return -1; }

        c->dir[i].tag = tag;
        c->dir[i].offset = off;
        c->dir[i].size = len;
    }

    (void)crc_acc;
    return 0;
}

int nova_cart_find(const nova_cart *c, uint32_t tag, const uint8_t **out, uint32_t *outlen)
{
    if (!c || !c->dir) return -1;
    for (int i = 0; i < c->chunk_cnt; i++) {
        if (c->dir[i].tag == tag) {
            if (out)    *out = c->base + c->dir[i].offset;
            if (outlen) *outlen = c->dir[i].size;
            return 0;
        }
    }
    return -1;
}

void nova_cart_close(nova_cart *c)
{
    if (!c) return;
    free(c->dir);
    c->dir = NULL;
    c->base = NULL;
}
