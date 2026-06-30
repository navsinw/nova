#include "nova.h"
#include <stdio.h>
#include <string.h>

/* bytecode -> text, used by the offline tools and debug dumps. */

struct opinfo { uint8_t op; const char *name; int imm; };

static const struct opinfo OPS[] = {
    { OP_NOP,"nop",0 }, { OP_HALT,"halt",0 }, { OP_PUSH,"push",4 }, { OP_PUSHB,"pushb",1 },
    { OP_POP,"pop",0 }, { OP_DUP,"dup",0 }, { OP_SWAP,"swap",0 },
    { OP_ADD,"add",0 }, { OP_SUB,"sub",0 }, { OP_MUL,"mul",0 }, { OP_DIV,"div",0 },
    { OP_MOD,"mod",0 }, { OP_AND,"and",0 }, { OP_OR,"or",0 }, { OP_XOR,"xor",0 },
    { OP_SHL,"shl",0 }, { OP_SHR,"shr",0 }, { OP_NEG,"neg",0 }, { OP_NOT,"not",0 },
    { OP_CMP,"cmp",0 }, { OP_ACCUM,"accum",0 },
    { OP_LD,"ld",1 }, { OP_ST,"st",1 }, { OP_LDM,"ldm",0 }, { OP_STM,"stm",0 },
    { OP_BANK,"bank",1 }, { OP_DMA,"dma",0 }, { OP_LREF,"lref",1 }, { OP_LDREF,"ldref",0 },
    { OP_EXEC,"exec",0 },
    { OP_JMP,"jmp",4 }, { OP_JZ,"jz",4 }, { OP_JNZ,"jnz",4 },
    { OP_CALL,"call",5 }, { OP_RET,"ret",0 }, { OP_SWITCH,"switch",0 },
    { OP_SPUSH,"spush",0 }, { OP_SPOP,"spop",0 }, { OP_SREAD,"sread",0 }, { OP_SSEEK,"sseek",0 },
    { OP_SPR,"spr",0 }, { OP_TILE,"tile",0 }, { OP_PALSET,"palset",0 }, { OP_PALCYC,"palcyc",0 },
    { OP_CAM,"cam",0 }, { OP_TEXT,"text",0 },
    { OP_VOICE,"voice",0 }, { OP_NOTE,"note",0 }, { OP_NOFF,"noff",0 }, { OP_PLAY,"play",1 },
    { OP_SPAWN,"spawn",0 }, { OP_KILL,"kill",0 }, { OP_TICK,"tick",0 },
    { OP_SAVE,"save",0 }, { OP_LOAD,"load",0 },
    { 0xff, NULL, 0 }
};

static const struct opinfo *find_op(uint8_t op)
{
    for (int i = 0; OPS[i].name; i++)
        if (OPS[i].op == op) return &OPS[i];
    return NULL;
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

int nova_disasm_one(const uint8_t *code, uint32_t size, uint32_t pc, char *out, int outsz)
{
    if (pc >= size) { if (outsz > 0) out[0] = 0; return 0; }
    uint8_t op = code[pc];
    const struct opinfo *oi = find_op(op);
    if (!oi) {
        snprintf(out, outsz, "%04x: .byte 0x%02x", pc, op);
        return 1;
    }
    if (oi->imm == 0) {
        snprintf(out, outsz, "%04x: %s", pc, oi->name);
        return 1;
    }
    if (oi->imm == 1) {
        uint8_t a = pc + 1 < size ? code[pc+1] : 0;
        snprintf(out, outsz, "%04x: %s %d", pc, oi->name, a);
        return 2;
    }
    if (oi->imm == 4) {
        uint32_t a = pc + 4 < size ? rd32(code + pc + 1) : 0;
        snprintf(out, outsz, "%04x: %s 0x%x", pc, oi->name, a);
        return 5;
    }
    if (oi->imm == 5) {
        uint32_t a = pc + 4 < size ? rd32(code + pc + 1) : 0;
        uint8_t n = pc + 5 < size ? code[pc+5] : 0;
        snprintf(out, outsz, "%04x: %s 0x%x, %d", pc, oi->name, a, n);
        return 6;
    }
    snprintf(out, outsz, "%04x: %s", pc, oi->name);
    return 1;
}

int nova_disasm_range(const uint8_t *code, uint32_t size, char *out, int outsz)
{
    uint32_t pc = 0;
    int used = 0;
    char line[96];
    while (pc < size && used < outsz - 1) {
        int adv = nova_disasm_one(code, size, pc, line, sizeof(line));
        if (adv <= 0) break;
        int n = snprintf(out + used, outsz - used, "%s\n", line);
        if (n <= 0) break;
        used += n;
        pc += (uint32_t)adv;
    }
    return used;
}
