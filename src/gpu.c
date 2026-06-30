#include "nova.h"

/* a second, simpler processor: a display-list interpreter. each command is a
   1-byte opcode followed by fixed operands. all reads are bounds-checked
   against the program length so a truncated list just stops. */

static int rd16s(const uint8_t *p, uint32_t len, uint32_t *ip, int *out)
{
    if (*ip + 2 > len) return 0;
    int v = (int)(int16_t)(p[*ip] | (p[*ip + 1] << 8));
    *ip += 2;
    *out = v;
    return 1;
}

static int rd8(const uint8_t *p, uint32_t len, uint32_t *ip, int *out)
{
    if (*ip + 1 > len) return 0;
    *out = p[(*ip)++];
    return 1;
}

int nova_gpu_run(nova_machine *mc, const uint8_t *prog, uint32_t len)
{
    if (!prog) return 0;
    uint32_t ip = 0;
    int executed = 0;
    int guard = 1 << 20;

    while (ip < len && guard-- > 0) {
        uint8_t op = prog[ip++];
        int a, b, c, d, e, f;
        switch (op) {
        case GOP_END:
            return executed;
        case GOP_CLEAR:
            if (!rd8(prog, len, &ip, &a)) return executed;
            nova_raster_clear(mc, (uint8_t)a);
            break;
        case GOP_PIXEL:
            if (!rd16s(prog,len,&ip,&a) || !rd16s(prog,len,&ip,&b) || !rd8(prog,len,&ip,&c)) return executed;
            nova_raster_pixel(mc, a, b, (uint8_t)c);
            break;
        case GOP_LINE:
            if (!rd16s(prog,len,&ip,&a)||!rd16s(prog,len,&ip,&b)||!rd16s(prog,len,&ip,&c)||
                !rd16s(prog,len,&ip,&d)||!rd8(prog,len,&ip,&e)) return executed;
            nova_raster_line(mc, a, b, c, d, (uint8_t)e);
            break;
        case GOP_RECT:
            if (!rd16s(prog,len,&ip,&a)||!rd16s(prog,len,&ip,&b)||!rd16s(prog,len,&ip,&c)||
                !rd16s(prog,len,&ip,&d)||!rd8(prog,len,&ip,&e)) return executed;
            nova_raster_rect(mc, a, b, c, d, (uint8_t)e);
            break;
        case GOP_FILL:
            if (!rd16s(prog,len,&ip,&a)||!rd16s(prog,len,&ip,&b)||!rd16s(prog,len,&ip,&c)||
                !rd16s(prog,len,&ip,&d)||!rd8(prog,len,&ip,&e)) return executed;
            nova_raster_fill(mc, a, b, c, d, (uint8_t)e);
            break;
        case GOP_CIRCLE:
            if (!rd16s(prog,len,&ip,&a)||!rd16s(prog,len,&ip,&b)||!rd16s(prog,len,&ip,&c)||
                !rd8(prog,len,&ip,&d)) return executed;
            nova_raster_circle(mc, a, b, c, (uint8_t)d);
            break;
        case GOP_SPRITE:
            if (!rd16s(prog,len,&ip,&a)||!rd16s(prog,len,&ip,&b)||!rd16s(prog,len,&ip,&c)) return executed;
            nova_gfx_sprite(mc, a, b, c);
            break;
        case GOP_SCALED:
            if (!rd16s(prog,len,&ip,&a)||!rd16s(prog,len,&ip,&b)||!rd16s(prog,len,&ip,&c)||
                !rd16s(prog,len,&ip,&d)||!rd16s(prog,len,&ip,&e)||!rd8(prog,len,&ip,&f)) return executed;
            nova_raster_blit_scaled(mc, a, b, c, d, e, f);
            break;
        case GOP_CAM:
            if (!rd16s(prog,len,&ip,&a) || !rd16s(prog,len,&ip,&b)) return executed;
            mc->gfx.cam_x = a; mc->gfx.cam_y = b;
            break;
        case GOP_PAL:
            if (!rd8(prog, len, &ip, &a)) return executed;
            nova_pal_set(mc, a);
            break;
        default:
            return executed;
        }
        executed++;
    }
    return executed;
}
