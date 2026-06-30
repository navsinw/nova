#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* nova_pack: build a cartridge from a manifest. each manifest line is
   "TAG path" (TAG is 1-4 chars, space-padded). chunk bodies are the raw
   contents of each file, laid out after the directory. */

typedef struct { uint32_t tag; uint8_t *data; uint32_t size; } Chunk;

static uint8_t *read_file(const char *path, uint32_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    uint8_t *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    *len = (uint32_t)fread(buf, 1, (size_t)n, f);
    fclose(f);
    return buf;
}

static uint32_t make_tag(const char *s)
{
    char t[4] = { ' ', ' ', ' ', ' ' };
    for (int i = 0; i < 4 && s[i]; i++) t[i] = s[i];
    return (uint32_t)(uint8_t)t[0] | ((uint32_t)(uint8_t)t[1] << 8) |
           ((uint32_t)(uint8_t)t[2] << 16) | ((uint32_t)(uint8_t)t[3] << 24);
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s manifest.txt out.nova\n", argv[0]); return 2; }
    FILE *mf = fopen(argv[1], "r");
    if (!mf) { fprintf(stderr, "cannot open manifest\n"); return 1; }

    Chunk chunks[64];
    int nch = 0;
    char line[512];
    while (fgets(line, sizeof(line), mf) && nch < 64) {
        char tagstr[32], path[480];
        if (sscanf(line, "%31s %479s", tagstr, path) != 2) continue;
        if (tagstr[0] == '#') continue;
        uint32_t sz = 0;
        uint8_t *d = read_file(path, &sz);
        if (!d) { fprintf(stderr, "cannot read %s\n", path); continue; }
        chunks[nch].tag = make_tag(tagstr);
        chunks[nch].data = d;
        chunks[nch].size = sz;
        nch++;
    }
    fclose(mf);
    if (nch == 0) { fprintf(stderr, "manifest empty\n"); return 1; }

    FILE *out = fopen(argv[2], "wb");
    if (!out) { fprintf(stderr, "cannot write\n"); return 1; }

    uint8_t hdr[18]; memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, "NOVA", 4);
    hdr[4] = 2;
    hdr[8] = (uint8_t)(nch & 0xff);
    hdr[9] = (uint8_t)((nch >> 8) & 0xff);
    fwrite(hdr, 1, 18, out);

    uint32_t off = 18 + 12 * (uint32_t)nch;
    for (int i = 0; i < nch; i++) {
        uint8_t e[12];
        uint32_t t = chunks[i].tag, o = off, s = chunks[i].size;
        e[0]=t&0xff; e[1]=(t>>8)&0xff; e[2]=(t>>16)&0xff; e[3]=(t>>24)&0xff;
        e[4]=o&0xff; e[5]=(o>>8)&0xff; e[6]=(o>>16)&0xff; e[7]=(o>>24)&0xff;
        e[8]=s&0xff; e[9]=(s>>8)&0xff; e[10]=(s>>16)&0xff; e[11]=(s>>24)&0xff;
        fwrite(e, 1, 12, out);
        off += s;
    }
    for (int i = 0; i < nch; i++) {
        fwrite(chunks[i].data, 1, chunks[i].size, out);
        free(chunks[i].data);
    }
    fclose(out);
    printf("packed %d chunks -> %s\n", nch, argv[2]);
    return 0;
}
