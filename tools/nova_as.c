#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* nova_as: a small two-pass assembler for NOVA-8 bytecode.
   supports a .code section (labels + instructions) and a .data section
   (.byte/.word), then links them into a CODE+DATA cartridge. */

typedef struct { const char *name; uint8_t op; int imm; } Mn;

static const Mn MN[] = {
    {"nop",OP_NOP,0},{"halt",OP_HALT,0},{"push",OP_PUSH,4},{"pushb",OP_PUSHB,1},
    {"pop",OP_POP,0},{"dup",OP_DUP,0},{"swap",OP_SWAP,0},
    {"add",OP_ADD,0},{"sub",OP_SUB,0},{"mul",OP_MUL,0},{"div",OP_DIV,0},
    {"mod",OP_MOD,0},{"and",OP_AND,0},{"or",OP_OR,0},{"xor",OP_XOR,0},
    {"shl",OP_SHL,0},{"shr",OP_SHR,0},{"neg",OP_NEG,0},{"not",OP_NOT,0},
    {"cmp",OP_CMP,0},{"accum",OP_ACCUM,0},
    {"ld",OP_LD,1},{"st",OP_ST,1},{"ldm",OP_LDM,0},{"stm",OP_STM,0},
    {"bank",OP_BANK,1},{"dma",OP_DMA,0},{"lref",OP_LREF,1},{"ldref",OP_LDREF,0},
    {"exec",OP_EXEC,0},
    {"jmp",OP_JMP,4},{"jz",OP_JZ,4},{"jnz",OP_JNZ,4},
    {"call",OP_CALL,5},{"ret",OP_RET,0},{"switch",OP_SWITCH,0},
    {"spush",OP_SPUSH,0},{"spop",OP_SPOP,0},{"sread",OP_SREAD,0},{"sseek",OP_SSEEK,0},
    {"spr",OP_SPR,0},{"tile",OP_TILE,0},{"palset",OP_PALSET,0},{"palcyc",OP_PALCYC,0},
    {"cam",OP_CAM,0},{"text",OP_TEXT,0},
    {"voice",OP_VOICE,0},{"note",OP_NOTE,0},{"noff",OP_NOFF,0},{"play",OP_PLAY,1},
    {"spawn",OP_SPAWN,0},{"kill",OP_KILL,0},{"tick",OP_TICK,0},
    {"save",OP_SAVE,0},{"load",OP_LOAD,0},
    {NULL,0,0}
};

static const Mn *lookup(const char *s)
{
    for (int i = 0; MN[i].name; i++)
        if (strcmp(MN[i].name, s) == 0) return &MN[i];
    return NULL;
}

typedef struct { char name[64]; int sect; uint32_t addr; } Sym;
static Sym  g_syms[2048];
static int  g_nsyms;

static int sym_find(const char *n)
{
    for (int i = 0; i < g_nsyms; i++)
        if (strcmp(g_syms[i].name, n) == 0) return i;
    return -1;
}

static int sym_add(const char *n, int sect, uint32_t addr)
{
    if (sym_find(n) >= 0) { fprintf(stderr, "dup label %s\n", n); return -1; }
    if (g_nsyms >= 2048) { fprintf(stderr, "too many labels\n"); return -1; }
    strncpy(g_syms[g_nsyms].name, n, 63);
    g_syms[g_nsyms].name[63] = 0;
    g_syms[g_nsyms].sect = sect;
    g_syms[g_nsyms].addr = addr;
    return g_nsyms++;
}

typedef struct { uint8_t *p; size_t n, cap; } Buf;

static void bput(Buf *b, uint8_t v)
{
    if (b->n >= b->cap) {
        b->cap = b->cap ? b->cap * 2 : 64;
        b->p = (uint8_t*)realloc(b->p, b->cap);
    }
    b->p[b->n++] = v;
}

static void bput32(Buf *b, uint32_t v)
{
    bput(b, v & 0xff); bput(b, (v >> 8) & 0xff);
    bput(b, (v >> 16) & 0xff); bput(b, (v >> 24) & 0xff);
}

static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

static int parse_num(const char *t, long *out)
{
    if (!*t) return -1;
    if (t[0] == '\'' && t[1] && t[2] == '\'') { *out = (unsigned char)t[1]; return 0; }
    char *end = NULL;
    long v = strtol(t, &end, 0);
    if (end && *end == 0) { *out = v; return 0; }
    return -1;
}

/* resolve an operand token to a value; numbers or label names (pass2) */
static int resolve(const char *t, long *out, int pass)
{
    if (parse_num(t, out) == 0) return 0;
    int idx = sym_find(t);
    if (idx >= 0) { *out = (long)g_syms[idx].addr; return 0; }
    if (pass == 2) { fprintf(stderr, "undefined symbol: %s\n", t); return -1; }
    *out = 0;
    return 0;
}

static int split_operands(char *rest, char ops[4][64])
{
    int n = 0;
    char *tok = strtok(rest, ",");
    while (tok && n < 4) {
        char *t = trim(tok);
        strncpy(ops[n], t, 63); ops[n][63] = 0;
        n++;
        tok = strtok(NULL, ",");
    }
    return n;
}

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t*)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    *len = fread(buf, 1, (size_t)n, f);
    buf[*len] = 0;
    fclose(f);
    return buf;
}

static void run_pass(char *src, int pass, Buf *code, Buf *data)
{
    int sect = 0;
    code->n = (pass == 1) ? code->n : 0;
    if (pass == 2) { code->n = 0; data->n = 0; }
    char *save = NULL;
    char *line = strtok_r(src, "\n", &save);
    char work[512];
    while (line) {
        strncpy(work, line, sizeof(work) - 1); work[sizeof(work) - 1] = 0;
        char *s = work; char *cm = strchr(s, ';'); if (cm) *cm = 0;
        s = trim(s);
        if (!*s) { line = strtok_r(NULL, "\n", &save); continue; }
        if (strcmp(s, ".code") == 0) { sect = 0; line = strtok_r(NULL,"\n",&save); continue; }
        if (strcmp(s, ".data") == 0) { sect = 1; line = strtok_r(NULL,"\n",&save); continue; }
        size_t L = strlen(s);
        if (s[L-1] == ':') {
            s[L-1] = 0;
            if (pass == 1) sym_add(trim(s), sect, sect==0?(uint32_t)code->n:(uint32_t)data->n);
            line = strtok_r(NULL,"\n",&save); continue;
        }
        char *sp = s; while (*sp && !isspace((unsigned char)*sp)) sp++;
        char mnem[64]; size_t mlen = (size_t)(sp - s); if (mlen > 63) mlen = 63;
        memcpy(mnem, s, mlen); mnem[mlen] = 0;
        char *rest = trim(sp);
        if (mnem[0] == '.') {
            char ops[4][64]; int no = split_operands(rest, ops);
            Buf *b = sect == 0 ? code : data;
            if (strcmp(mnem, ".byte") == 0) {
                for (int i=0;i<no;i++){ long v=0; resolve(ops[i],&v,pass); bput(b,(uint8_t)v); }
            } else if (strcmp(mnem, ".word") == 0) {
                for (int i=0;i<no;i++){ long v=0; resolve(ops[i],&v,pass); bput32(b,(uint32_t)v); }
            } else if (strcmp(mnem, ".space") == 0 && no >= 1) {
                long v=0; resolve(ops[0],&v,pass); for (long k=0;k<v;k++) bput(b,0);
            }
            line = strtok_r(NULL,"\n",&save); continue;
        }
        const Mn *m = lookup(mnem);
        if (!m) { if (pass==2) fprintf(stderr,"unknown op %s\n",mnem); line=strtok_r(NULL,"\n",&save); continue; }
        bput(code, m->op);
        char ops[4][64]; int no = split_operands(rest, ops);
        if (m->imm == 1) { long v=0; if(no)resolve(ops[0],&v,pass); bput(code,(uint8_t)v); }
        else if (m->imm == 4) { long v=0; if(no)resolve(ops[0],&v,pass); bput32(code,(uint32_t)v); }
        else if (m->imm == 5) {
            long a=0,b=0; if(no)resolve(ops[0],&a,pass); if(no>=2)resolve(ops[1],&b,pass);
            bput32(code,(uint32_t)a); bput(code,(uint8_t)b);
        }
        line = strtok_r(NULL,"\n",&save);
    }
}

static int write_cart(const char *path, Buf *code, Buf *data)
{
    int nchunks = data->n ? 2 : 1;
    uint32_t body = 18 + 12 * (uint32_t)nchunks;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    uint8_t hdr[18]; memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, "NOVA", 4);
    hdr[4] = 2;
    hdr[8] = (uint8_t)nchunks;
    fwrite(hdr, 1, 18, f);
    uint32_t off = body;
    uint8_t e[12];
    /* CODE */
    memcpy(e, "CODE", 4);
    e[4]=off&0xff; e[5]=(off>>8)&0xff; e[6]=(off>>16)&0xff; e[7]=(off>>24)&0xff;
    uint32_t sz=(uint32_t)code->n;
    e[8]=sz&0xff; e[9]=(sz>>8)&0xff; e[10]=(sz>>16)&0xff; e[11]=(sz>>24)&0xff;
    fwrite(e,1,12,f);
    off += sz;
    if (nchunks == 2) {
        memcpy(e, "DATA", 4);
        e[4]=off&0xff; e[5]=(off>>8)&0xff; e[6]=(off>>16)&0xff; e[7]=(off>>24)&0xff;
        uint32_t ds=(uint32_t)data->n;
        e[8]=ds&0xff; e[9]=(ds>>8)&0xff; e[10]=(ds>>16)&0xff; e[11]=(ds>>24)&0xff;
        fwrite(e,1,12,f);
    }
    fwrite(code->p, 1, code->n, f);
    if (data->n) fwrite(data->p, 1, data->n, f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s in.s out.nova\n", argv[0]); return 2; }
    size_t len = 0;
    uint8_t *src = read_file(argv[1], &len);
    if (!src) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }

    Buf code = {0}, data = {0};
    /* pass 1: gather labels (work on a copy because strtok mutates) */
    char *copy1 = strdup((char*)src);
    run_pass(copy1, 1, &code, &data);
    free(copy1);
    /* pass 2: emit */
    char *copy2 = strdup((char*)src);
    run_pass(copy2, 2, &code, &data);
    free(copy2);

    if (write_cart(argv[2], &code, &data) != 0) {
        fprintf(stderr, "write failed\n"); free(src); return 1;
    }
    printf("assembled %s: code=%zu data=%zu -> %s\n", argv[1], code.n, data.n, argv[2]);
    free(code.p); free(data.p); free(src);
    return 0;
}
