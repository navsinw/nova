#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* nova_cc: a tiny C-like language that compiles to NOVA-8 bytecode.
   grammar:
     program := stmt*
     stmt    := 'var' id '=' expr ';' | id '=' expr ';'
              | 'if' '(' cond ')' block ['else' block]
              | 'while' '(' cond ')' block
              | id '(' args ')' ';' | 'halt' ';'
     cond    := add relop add
     expr    := add ; add := mul (('+'|'-') mul)* ; mul := prim (('*'|'/'|'%') prim)*
     prim    := number | id | 'peek' '(' expr ')' | '(' expr ')'
*/

enum { T_EOF, T_NUM, T_ID, T_PUNCT };
typedef struct { int type; long num; char str[64]; } Tok;

static Tok  T[16384];
static int  NT, P;

static void lex(const char *s)
{
    NT = 0;
    while (*s && NT < 16383) {
        if (isspace((unsigned char)*s)) { s++; continue; }
        if (s[0] == '/' && s[1] == '/') { while (*s && *s != '\n') s++; continue; }
        if (isdigit((unsigned char)*s) || (s[0] == '-' && isdigit((unsigned char)s[1]))) {
            char *end; long v = strtol(s, &end, 0);
            T[NT].type = T_NUM; T[NT].num = v; NT++; s = end; continue;
        }
        if (isalpha((unsigned char)*s) || *s == '_') {
            int n = 0; char buf[64];
            while ((isalnum((unsigned char)*s) || *s == '_') && n < 63) buf[n++] = *s++;
            buf[n] = 0;
            T[NT].type = T_ID; strncpy(T[NT].str, buf, 63); T[NT].str[63] = 0; NT++;
            continue;
        }
        /* two-char operators */
        if ((s[0] == '=' && s[1] == '=') || (s[0] == '!' && s[1] == '=') ||
            (s[0] == '<' && s[1] == '=') || (s[0] == '>' && s[1] == '=')) {
            T[NT].type = T_PUNCT; T[NT].str[0] = s[0]; T[NT].str[1] = s[1]; T[NT].str[2] = 0;
            NT++; s += 2; continue;
        }
        T[NT].type = T_PUNCT; T[NT].str[0] = *s; T[NT].str[1] = 0; NT++; s++;
    }
    T[NT].type = T_EOF; T[NT].str[0] = 0;
}

static Tok *cur(void) { return &T[P]; }
static int  is_punct(const char *p) { return T[P].type == T_PUNCT && strcmp(T[P].str, p) == 0; }
static int  is_id(const char *k) { return T[P].type == T_ID && strcmp(T[P].str, k) == 0; }

static void expect(const char *p)
{
    if (!is_punct(p)) { fprintf(stderr, "expected '%s' near token %d ('%s')\n", p, P, T[P].str); exit(1); }
    P++;
}

/* --- codegen --- */
typedef struct { uint8_t *p; size_t n, cap; } Buf;
static Buf CODE;

static void emit(uint8_t v)
{
    if (CODE.n >= CODE.cap) { CODE.cap = CODE.cap ? CODE.cap * 2 : 128; CODE.p = realloc(CODE.p, CODE.cap); }
    CODE.p[CODE.n++] = v;
}
static void emit32(uint32_t v) { emit(v & 0xff); emit((v>>8)&0xff); emit((v>>16)&0xff); emit((v>>24)&0xff); }
static void push_imm(long v) { emit(OP_PUSH); emit32((uint32_t)v); }

static int emit_branch(uint8_t op)
{
    emit(op);
    int pos = (int)CODE.n;
    emit32(0);
    return pos;
}
static void patch(int pos, uint32_t target)
{
    CODE.p[pos] = target & 0xff;
    CODE.p[pos+1] = (target >> 8) & 0xff;
    CODE.p[pos+2] = (target >> 16) & 0xff;
    CODE.p[pos+3] = (target >> 24) & 0xff;
}

static char regs[16][64];
static int  nregs;

static int reg_of(const char *name)
{
    for (int i = 0; i < nregs; i++)
        if (strcmp(regs[i], name) == 0) return i;
    if (nregs >= 16) { fprintf(stderr, "out of registers (max 16 vars)\n"); exit(1); }
    strncpy(regs[nregs], name, 63); regs[nregs][63] = 0;
    return nregs++;
}

static void gen_expr(void);

struct builtin { const char *name; uint8_t op; int args; int yields; };
static const struct builtin BI[] = {
    {"spr",OP_SPR,3,0},{"tile",OP_TILE,4,0},{"pal",OP_PALSET,1,0},{"palcyc",OP_PALCYC,2,0},
    {"cam",OP_CAM,2,0},{"voice",OP_VOICE,2,0},{"note",OP_NOTE,2,0},{"noff",OP_NOFF,1,0},
    {"poke",OP_STM,2,0},{"spawn",OP_SPAWN,3,1},{"kill",OP_KILL,1,0},{"dma",OP_DMA,3,0},
    {NULL,0,0,0}
};

static const struct builtin *find_builtin(const char *n)
{
    for (int i = 0; BI[i].name; i++)
        if (strcmp(BI[i].name, n) == 0) return &BI[i];
    return NULL;
}

static void gen_prim(void)
{
    if (cur()->type == T_NUM) { push_imm(cur()->num); P++; return; }
    if (is_punct("(")) { P++; gen_expr(); expect(")"); return; }
    if (cur()->type == T_ID) {
        if (is_id("peek")) { P++; expect("("); gen_expr(); expect(")"); emit(OP_LDM); return; }
        int r = reg_of(cur()->str); P++;
        emit(OP_LD); emit((uint8_t)r);
        return;
    }
    fprintf(stderr, "bad primary near token %d ('%s')\n", P, T[P].str);
    exit(1);
}

static void gen_mul(void)
{
    gen_prim();
    while (is_punct("*") || is_punct("/") || is_punct("%")) {
        char o = T[P].str[0]; P++;
        gen_prim();
        emit(o == '*' ? OP_MUL : (o == '/' ? OP_DIV : OP_MOD));
    }
}

static void gen_add(void)
{
    gen_mul();
    while (is_punct("+") || is_punct("-")) {
        char o = T[P].str[0]; P++;
        gen_mul();
        emit(o == '+' ? OP_ADD : OP_SUB);
    }
}

static void gen_expr(void) { gen_add(); }

/* compile a relational condition; emit a forward jump taken when the
   condition is FALSE; return the patch position of that jump's target. */
static int gen_cond_falsejump(void)
{
    gen_add();
    char op[3] = {0,0,0};
    if (cur()->type == T_PUNCT) { op[0] = T[P].str[0]; op[1] = T[P].str[1]; }
    P++;
    gen_add();
    emit(OP_CMP);
    if (strcmp(op, "==") == 0) return emit_branch(OP_JNZ);
    if (strcmp(op, "!=") == 0) return emit_branch(OP_JZ);
    if (strcmp(op, "<") == 0)  { push_imm(1);  emit(OP_ADD); return emit_branch(OP_JNZ); }
    if (strcmp(op, ">") == 0)  { push_imm(-1); emit(OP_ADD); return emit_branch(OP_JNZ); }
    if (strcmp(op, "<=") == 0) { push_imm(-1); emit(OP_ADD); return emit_branch(OP_JZ); }
    if (strcmp(op, ">=") == 0) { push_imm(1);  emit(OP_ADD); return emit_branch(OP_JZ); }
    fprintf(stderr, "bad relational operator '%s'\n", op);
    exit(1);
}

static void gen_block(void);

static void gen_stmt(void)
{
    if (is_id("halt")) { P++; expect(";"); emit(OP_HALT); return; }
    if (is_id("tick")) { P++; expect("("); expect(")"); expect(";"); emit(OP_TICK); return; }

    if (is_id("var")) {
        P++;
        if (cur()->type != T_ID) { fprintf(stderr, "var needs name\n"); exit(1); }
        int r = reg_of(cur()->str); P++;
        expect("="); gen_expr(); expect(";");
        emit(OP_ST); emit((uint8_t)r);
        return;
    }

    if (is_id("if")) {
        P++; expect("(");
        int pelse = gen_cond_falsejump();
        expect(")");
        gen_block();
        if (is_id("else")) {
            P++;
            int pend = emit_branch(OP_JMP);
            patch(pelse, (uint32_t)CODE.n);
            gen_block();
            patch(pend, (uint32_t)CODE.n);
        } else {
            patch(pelse, (uint32_t)CODE.n);
        }
        return;
    }

    if (is_id("while")) {
        P++; expect("(");
        int top = (int)CODE.n;
        int pend = gen_cond_falsejump();
        expect(")");
        gen_block();
        emit(OP_JMP); emit32((uint32_t)top);
        patch(pend, (uint32_t)CODE.n);
        return;
    }

    if (cur()->type == T_ID) {
        char name[64]; strncpy(name, cur()->str, 63); name[63] = 0;
        /* assignment or builtin call */
        if (T[P+1].type == T_PUNCT && strcmp(T[P+1].str, "(") == 0) {
            const struct builtin *b = find_builtin(name);
            if (!b) { fprintf(stderr, "unknown call '%s'\n", name); exit(1); }
            P += 2; /* name ( */
            int argc = 0;
            if (!is_punct(")")) {
                gen_expr(); argc++;
                while (is_punct(",")) { P++; gen_expr(); argc++; }
            }
            expect(")"); expect(";");
            if (argc != b->args) fprintf(stderr, "warning: %s expects %d args, got %d\n", name, b->args, argc);
            emit(b->op);
            if (b->yields) emit(OP_POP);
            return;
        }
        int r = reg_of(name); P++;
        expect("="); gen_expr(); expect(";");
        emit(OP_ST); emit((uint8_t)r);
        return;
    }

    fprintf(stderr, "unexpected token %d ('%s')\n", P, T[P].str);
    exit(1);
}

static void gen_block(void)
{
    if (is_punct("{")) {
        P++;
        while (!is_punct("}") && cur()->type != T_EOF) gen_stmt();
        expect("}");
    } else {
        gen_stmt();
    }
}

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    uint8_t *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    *len = fread(buf, 1, (size_t)n, f); buf[*len] = 0; fclose(f);
    return buf;
}

static int write_cart(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    uint8_t hdr[18]; memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, "NOVA", 4); hdr[4] = 2; hdr[8] = 1; /* 1 chunk */
    fwrite(hdr, 1, 18, f);
    uint32_t off = 18 + 12;
    uint8_t e[12]; memcpy(e, "CODE", 4);
    e[4]=off&0xff; e[5]=(off>>8)&0xff; e[6]=(off>>16)&0xff; e[7]=(off>>24)&0xff;
    uint32_t sz=(uint32_t)CODE.n;
    e[8]=sz&0xff; e[9]=(sz>>8)&0xff; e[10]=(sz>>16)&0xff; e[11]=(sz>>24)&0xff;
    fwrite(e, 1, 12, f);
    fwrite(CODE.p, 1, CODE.n, f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s in.nc out.nova\n", argv[0]); return 2; }
    size_t len = 0;
    uint8_t *src = read_file(argv[1], &len);
    if (!src) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }

    lex((const char*)src);
    P = 0;
    while (cur()->type != T_EOF) gen_stmt();
    emit(OP_HALT);

    if (write_cart(argv[2]) != 0) { fprintf(stderr, "write failed\n"); free(src); return 1; }
    printf("compiled %s: %zu code bytes, %d vars -> %s\n", argv[1], CODE.n, nregs, argv[2]);
    free(CODE.p); free(src);
    return 0;
}
