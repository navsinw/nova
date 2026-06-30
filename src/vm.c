#include "nova.h"
#include <stdlib.h>
#include <string.h>

int nova_vm_init(nova_vm *vm, nova_machine *mc)
{
    memset(vm, 0, sizeof(*vm));
    vm->stack_cap = NOVA_STACK_INIT;
    vm->stack = (int32_t*)malloc(sizeof(int32_t) * vm->stack_cap);
    if (!vm->stack) return -1;
    vm->frame_cap = NOVA_FRAME_INIT;
    vm->frames = (nova_frame*)calloc(vm->frame_cap, sizeof(nova_frame));
    if (!vm->frames) { free(vm->stack); return -1; }

    const uint8_t *code; uint32_t clen;
    if (nova_cart_find(&mc->cart, TAG_CODE, &code, &clen) != 0) return -1;
    vm->code = code;
    vm->code_size = clen;

    const uint8_t *d; uint32_t dl;
    if (nova_cart_find(&mc->cart, TAG_DATA, &d, &dl) == 0) {
        vm->data = d; vm->data_size = dl;
    }
    vm->pc = mc->cart.entry_pc;
    vm->running = 1;
    return 0;
}

void nova_vm_free(nova_vm *vm)
{
    if (!vm) return;
    for (size_t i = 0; i < vm->fp; i++)
        free(vm->frames[i].locals);
    for (int i = 0; i < vm->cur_depth; i++)
        if (vm->cursors[i].owned) free(vm->cursors[i].buf);
    free(vm->stack);
    free(vm->frames);
    vm->stack = NULL; vm->frames = NULL;
}

static void vm_push(nova_vm *vm, int32_t v)
{
    if (vm->sp == vm->stack_cap) {
        size_t nc = vm->stack_cap * 2;
        int32_t *ns = (int32_t*)realloc(vm->stack, sizeof(int32_t) * nc);
        if (!ns) return;
        vm->stack = ns;
        vm->stack_cap = nc;
    }
    vm->stack[vm->sp++] = v;
}

static int32_t vm_pop(nova_vm *vm)
{
    if (vm->sp == 0) return 0;
    return vm->stack[--vm->sp];
}

static const uint8_t *vm_src(nova_machine *mc, uint32_t *limit)
{
    nova_vm *vm = &mc->vm;
    if (vm->in_ram_exec) {
        *limit = vm->code_size;
        return mc->mem.ram + mc->mem.exec_base;
    }
    *limit = vm->code_size;
    return vm->code;
}

static uint32_t fetch32(nova_machine *mc)
{
    nova_vm *vm = &mc->vm;
    uint32_t limit; const uint8_t *s = vm_src(mc, &limit);
    if (vm->pc + 4 > limit) { vm->running = 0; return 0; }
    uint32_t v = (uint32_t)s[vm->pc] | ((uint32_t)s[vm->pc+1]<<8)
               | ((uint32_t)s[vm->pc+2]<<16) | ((uint32_t)s[vm->pc+3]<<24);
    vm->pc += 4;
    return v;
}

static uint8_t fetch8(nova_machine *mc)
{
    nova_vm *vm = &mc->vm;
    uint32_t limit; const uint8_t *s = vm_src(mc, &limit);
    if (vm->pc >= limit) { vm->running = 0; return 0; }
    return s[vm->pc++];
}

int nova_vm_step(nova_machine *mc)
{
    nova_vm *vm = &mc->vm;
    uint32_t limit; const uint8_t *s = vm_src(mc, &limit);
    if (!vm->running || vm->pc >= limit) { vm->running = 0; return 0; }

    uint8_t op = s[vm->pc++];

    switch (op) {
    case OP_NOP: break;
    case OP_HALT: vm->running = 0; break;

    case OP_PUSH:  vm_push(vm, (int32_t)fetch32(mc)); break;
    case OP_PUSHB: vm_push(vm, (int8_t)fetch8(mc)); break;
    case OP_POP:   (void)vm_pop(vm); break;
    case OP_DUP: { int32_t a = vm_pop(vm); vm_push(vm, a); vm_push(vm, a); break; }
    case OP_SWAP:{ int32_t a = vm_pop(vm), b = vm_pop(vm); vm_push(vm, a); vm_push(vm, b); break; }

    case OP_ADD: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,a+b); break; }
    case OP_SUB: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,a-b); break; }
    case OP_MUL: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,a*b); break; }
    case OP_DIV: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm, b?a/b:0); break; }
    case OP_MOD: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm, b?a%b:0); break; }
    case OP_AND: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,a&b); break; }
    case OP_OR:  { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,a|b); break; }
    case OP_XOR: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,a^b); break; }
    case OP_SHL: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,a<<(b&31)); break; }
    case OP_SHR: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm,(int32_t)((uint32_t)a>>(b&31))); break; }
    case OP_NEG: vm_push(vm, -vm_pop(vm)); break;
    case OP_NOT: vm_push(vm, ~vm_pop(vm)); break;
    case OP_CMP: { int32_t b=vm_pop(vm),a=vm_pop(vm); vm_push(vm, (a>b)-(a<b)); break; }

    case OP_ACCUM: {
        if (vm->sp == 0) break;
        int32_t *slot = &vm->stack[vm->sp - 1];
        vm_push(vm, 0);
        *slot = *slot + 1;
        break;
    }

    case OP_LD: { uint8_t r = fetch8(mc); vm_push(vm, vm->reg[r & (NOVA_NUM_REGS-1)]); break; }
    case OP_ST: { uint8_t r = fetch8(mc); vm->reg[r & (NOVA_NUM_REGS-1)] = vm_pop(vm); break; }

    case OP_LDM: { uint32_t a = (uint32_t)vm_pop(vm); vm_push(vm, nova_mem_load(&mc->mem, a)); break; }
    case OP_STM: { int32_t v = vm_pop(vm); uint32_t a = (uint32_t)vm_pop(vm); nova_mem_store(&mc->mem, a, v); break; }
    case OP_BANK:{ uint8_t b = fetch8(mc); nova_mem_bank(&mc->mem, b); break; }
    case OP_DMA: { uint32_t len=(uint32_t)vm_pop(vm), src=(uint32_t)vm_pop(vm), dst=(uint32_t)vm_pop(vm);
                   nova_mem_dma(&mc->mem, dst, src, len); break; }

    case OP_LREF: {
        uint8_t i = fetch8(mc);
        if (vm->fp == 0) break;
        nova_frame *fr = &vm->frames[vm->fp - 1];
        if (i < fr->nlocals && vm->nhandles < NOVA_NUM_REGS) {
            int h = vm->nhandles++;
            vm->handles[h].base = &fr->locals[i];
            vm->handles[h].index = i;
            vm->handles[h].live = 1;
            vm_push(vm, h);
        }
        break;
    }
    case OP_LDREF: {
        int h = vm_pop(vm);
        if (h >= 0 && h < vm->nhandles)
            vm_push(vm, *vm->handles[h].base);
        break;
    }

    case OP_EXEC: {
        uint32_t base = (uint32_t)vm_pop(vm);
        uint32_t sz   = (uint32_t)vm_pop(vm);
        mc->mem.exec_base = base;
        mc->mem.exec_size = sz;
        vm->in_ram_exec = 1;
        vm->pc = 0;
        break;
    }

    case OP_JMP: { uint32_t t = fetch32(mc); if (t < vm->code_size) vm->pc = t; else vm->running = 0; break; }
    case OP_JZ:  { uint32_t t = fetch32(mc); if (vm_pop(vm)==0){ if(t<vm->code_size) vm->pc=t; } break; }
    case OP_JNZ: { uint32_t t = fetch32(mc); if (vm_pop(vm)!=0){ if(t<vm->code_size) vm->pc=t; } break; }

    case OP_CALL: {
        uint32_t t = fetch32(mc);
        uint8_t nl = fetch8(mc);
        if (vm->fp >= vm->frame_cap) {
            size_t nc = vm->frame_cap * 2;
            nova_frame *nf = (nova_frame*)realloc(vm->frames, nc * sizeof(nova_frame));
            if (!nf) { vm->running = 0; break; }
            vm->frames = nf; vm->frame_cap = nc;
        }
        nova_frame *fr = &vm->frames[vm->fp++];
        fr->ret_pc = vm->pc;
        fr->nlocals = nl;
        fr->locals = (int32_t*)calloc(nl ? nl : 1, sizeof(int32_t));
        if (t < vm->code_size) vm->pc = t; else vm->running = 0;
        break;
    }
    case OP_RET: {
        if (vm->fp == 0) { vm->running = 0; break; }
        nova_frame *fr = &vm->frames[--vm->fp];
        vm->pc = fr->ret_pc;
        free(fr->locals);
        fr->locals = NULL;
        break;
    }

    case OP_SWITCH: {
        int32_t idx = vm_pop(vm);
        if (!vm->data || vm->data_size < 4) { break; }
        uint32_t n = (uint32_t)vm->data[0] | ((uint32_t)vm->data[1]<<8)
                   | ((uint32_t)vm->data[2]<<16) | ((uint32_t)vm->data[3]<<24);
        if (idx >= 0 && (uint32_t)idx < n) {
            uint32_t off = 4 + (uint32_t)idx * 4;
            if (off + 4 <= vm->data_size) {
                uint32_t t = (uint32_t)vm->data[off] | ((uint32_t)vm->data[off+1]<<8)
                           | ((uint32_t)vm->data[off+2]<<16) | ((uint32_t)vm->data[off+3]<<24);
                if (t < vm->code_size) vm->pc = t;
            }
        }
        break;
    }

    case OP_SPUSH: {
        if (vm->cur_depth >= NOVA_STREAM_DEPTH) break;
        nova_cursor *nc = &vm->cursors[vm->cur_depth];
        if (vm->cur_depth > 0) {
            *nc = vm->cursors[vm->cur_depth - 1];
            nc->pos = 0;
        } else {
            uint32_t wsz = vm->data_size < 64 ? vm->data_size : 64;
            if (wsz == 0) wsz = 1;
            nc->buf = (uint8_t*)malloc(wsz);
            if (vm->data) memcpy(nc->buf, vm->data, vm->data_size < wsz ? vm->data_size : wsz);
            nc->size = wsz; nc->pos = 0; nc->owned = 1;
        }
        vm->cur_depth++;
        break;
    }
    case OP_SPOP: {
        if (vm->cur_depth <= 0) break;
        vm->cur_depth--;
        nova_cursor *c = &vm->cursors[vm->cur_depth];
        if (c->owned) free(c->buf);
        break;
    }
    case OP_SREAD: {
        if (vm->cur_depth <= 0) break;
        nova_cursor *c = &vm->cursors[vm->cur_depth - 1];
        if (c->pos < c->size) vm_push(vm, c->buf[c->pos++]);
        else vm_push(vm, -1);
        break;
    }
    case OP_SSEEK: {
        if (vm->cur_depth <= 0) break;
        nova_cursor *c = &vm->cursors[vm->cur_depth - 1];
        int32_t d = vm_pop(vm);
        c->pos = (uint32_t)((int32_t)c->pos + d);
        break;
    }

    case OP_SPR:   { int y=vm_pop(vm),x=vm_pop(vm),id=vm_pop(vm); nova_gfx_sprite(mc,id,x,y); break; }
    case OP_TILE:  { int r=vm_pop(vm),c=vm_pop(vm),my=vm_pop(vm),mx=vm_pop(vm); nova_gfx_tile(mc,mx,my,c,r); break; }
    case OP_PALSET:{ int b=vm_pop(vm); nova_pal_set(mc,b); break; }
    case OP_PALCYC:{ int hi=vm_pop(vm),lo=vm_pop(vm); nova_pal_cycle(mc,lo,hi); break; }
    case OP_CAM:   { int dy=vm_pop(vm),dx=vm_pop(vm); mc->gfx.cam_x=dx; mc->gfx.cam_y=dy; break; }
    case OP_TEXT:  { int y=vm_pop(vm),x=vm_pop(vm); uint32_t a=(uint32_t)vm_pop(vm); nova_gfx_text(mc,a,x,y); break; }

    case OP_VOICE: { int instr=vm_pop(vm),ch=vm_pop(vm); nova_synth_voice(mc,ch,instr); break; }
    case OP_NOTE:  { int p=vm_pop(vm),ch=vm_pop(vm); nova_synth_note(mc,ch,p); break; }
    case OP_NOFF:  { int ch=vm_pop(vm); nova_synth_noteoff(mc,ch); break; }
    case OP_PLAY:  { uint8_t o=fetch8(mc); nova_tracker_play(mc,o); break; }

    case OP_SPAWN: { int y=vm_pop(vm),x=vm_pop(vm),id=vm_pop(vm); vm_push(vm, nova_world_spawn(mc,id,x,y)); break; }
    case OP_KILL:  { int h=vm_pop(vm); nova_world_kill(mc,h); break; }
    case OP_TICK:  nova_machine_tick(mc); break;
    case OP_SAVE:  nova_savestate_save(mc); break;
    case OP_LOAD:  if (mc->save_region) nova_savestate_load(mc, mc->save_region, mc->save_size); break;

    default: vm->running = 0; break;
    }
    return vm->running;
}

int nova_vm_run(nova_machine *mc, int max_steps)
{
    int n = 0;
    while (mc->vm.running && n < max_steps) {
        nova_vm_step(mc);
        n++;
    }
    return n;
}
