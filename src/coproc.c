#include "nova.h"

/* a second, register-based command processor (think "blitter chip"). it has
   16 registers and a compact instruction stream; all framebuffer access goes
   through the clipped rasterizer. reads are bounds-checked against len. */

enum {
    CP_END = 0, CP_SET, CP_MOV, CP_ADD, CP_SUB, CP_MUL,
    CP_PIX, CP_HLINE, CP_VLINE, CP_RECT, CP_LOOP, CP_NEXT
};

int nova_coproc_run(nova_machine *mc, const uint8_t *prog, uint32_t len)
{
    int32_t reg[16];
    for (int i = 0; i < 16; i++) reg[i] = 0;

    uint32_t ip = 0;
    int loop_ip = -1, loop_count = 0;
    int executed = 0;
    int guard = 1 << 20;

    while (ip < len && guard-- > 0) {
        uint8_t op = prog[ip++];
        switch (op) {
        case CP_END:
            return executed;
        case CP_SET: {
            if (ip + 5 > len) return executed;
            int r = prog[ip] & 15;
            int32_t v = (int32_t)(prog[ip+1] | (prog[ip+2]<<8) | (prog[ip+3]<<16) | (prog[ip+4]<<24));
            ip += 5;
            reg[r] = v;
            break;
        }
        case CP_MOV: {
            if (ip + 2 > len) return executed;
            reg[prog[ip] & 15] = reg[prog[ip+1] & 15];
            ip += 2;
            break;
        }
        case CP_ADD: {
            if (ip + 2 > len) return executed;
            reg[prog[ip] & 15] += reg[prog[ip+1] & 15];
            ip += 2;
            break;
        }
        case CP_SUB: {
            if (ip + 2 > len) return executed;
            reg[prog[ip] & 15] -= reg[prog[ip+1] & 15];
            ip += 2;
            break;
        }
        case CP_MUL: {
            if (ip + 2 > len) return executed;
            reg[prog[ip] & 15] *= reg[prog[ip+1] & 15];
            ip += 2;
            break;
        }
        case CP_PIX: {
            if (ip + 3 > len) return executed;
            nova_raster_pixel(mc, reg[prog[ip] & 15], reg[prog[ip+1] & 15], (uint8_t)reg[prog[ip+2] & 15]);
            ip += 3;
            break;
        }
        case CP_HLINE: {
            if (ip + 4 > len) return executed;
            nova_raster_hline(mc, reg[prog[ip]&15], reg[prog[ip+1]&15], reg[prog[ip+2]&15], (uint8_t)reg[prog[ip+3]&15]);
            ip += 4;
            break;
        }
        case CP_VLINE: {
            if (ip + 4 > len) return executed;
            nova_raster_vline(mc, reg[prog[ip]&15], reg[prog[ip+1]&15], reg[prog[ip+2]&15], (uint8_t)reg[prog[ip+3]&15]);
            ip += 4;
            break;
        }
        case CP_RECT: {
            if (ip + 5 > len) return executed;
            nova_raster_rect(mc, reg[prog[ip]&15], reg[prog[ip+1]&15], reg[prog[ip+2]&15], reg[prog[ip+3]&15], (uint8_t)reg[prog[ip+4]&15]);
            ip += 5;
            break;
        }
        case CP_LOOP: {
            if (ip + 1 > len) return executed;
            loop_count = prog[ip++];
            loop_ip = (int)ip;
            break;
        }
        case CP_NEXT: {
            if (loop_count > 1 && loop_ip >= 0) {
                loop_count--;
                ip = (uint32_t)loop_ip;
            }
            break;
        }
        default:
            return executed;
        }
        executed++;
    }
    return executed;
}
