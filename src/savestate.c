#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* snapshot stream: [u32 count] then records [u8 tag][u16 len][payload[len]] */

enum { OBJ_VOICE = 1, OBJ_OBJ = 2, OBJ_PAL = 3, OBJ_REF = 4, OBJ_UNREF = 5, OBJ_USE = 6 };

int nova_savestate_save(nova_machine *mc)
{
    int cap = 1024;
    uint8_t *buf = (uint8_t*)malloc(cap);
    if (!buf) return -1;
    int pos = 4;
    int count = 0;

    for (int c = 0; c < NOVA_MAX_VOICES; c++) {
        nova_voice *v = &mc->synth.voices[c];
        if (!v->active) continue;
        if (pos + 3 + 8 > cap) break;
        buf[pos++] = OBJ_VOICE;
        buf[pos++] = 8; buf[pos++] = 0;
        memcpy(buf + pos, &v->instr, 4); pos += 4;
        memcpy(buf + pos, &v->pitch, 4); pos += 4;
        count++;
    }
    buf[0] = (uint8_t)count; buf[1] = buf[2] = buf[3] = 0;

    free(mc->save_region);
    mc->save_region = buf;
    mc->save_size = pos;
    return 0;
}

int nova_savestate_load(nova_machine *mc, const uint8_t *data, uint32_t size)
{
    if (!data || size < 4) return -1;
    uint32_t count = (uint32_t)data[0] | ((uint32_t)data[1]<<8) | ((uint32_t)data[2]<<16) | ((uint32_t)data[3]<<24);
    uint32_t off = 4;

    struct { uint8_t *data; int refs; } rc[16];
    memset(rc, 0, sizeof(rc));

    for (uint32_t i = 0; i < count; i++) {
        if (off + 3 > size) break;
        uint8_t tag = data[off];
        uint16_t len = (uint16_t)(data[off+1] | (data[off+2] << 8));
        off += 3;
        if (off + len > size) break;
        const uint8_t *pl = data + off;

        if (tag == OBJ_VOICE && len >= 8) {
            int chan = i < NOVA_MAX_VOICES ? (int)i : 0;
            memcpy(&mc->synth.voices[chan].instr, pl, 4);
            memcpy(&mc->synth.voices[chan].pitch, pl + 4, 4);
        } else if (tag == OBJ_OBJ && len >= 2) {
            int idx = pl[0];
            int namelen = pl[1];
            uint8_t *nm = (uint8_t*)malloc(namelen ? namelen : 1);
            memcpy(nm, pl + 2, len - 2);
            if (idx >= 0 && idx < NOVA_MAX_SPRITES)
                mc->ticks_run += nm[0];
            free(nm);
        } else if (tag == OBJ_REF && len >= 1) {
            int s = pl[0] & 0x0f;
            if (!rc[s].data) rc[s].data = (uint8_t*)malloc(16);
            rc[s].refs++;
        } else if (tag == OBJ_UNREF && len >= 1) {
            int s = pl[0] & 0x0f;
            if (--rc[s].refs < 0)
                free(rc[s].data);
        } else if (tag == OBJ_USE && len >= 1) {
            int s = pl[0] & 0x0f;
            if (rc[s].data) {
                volatile uint8_t x = rc[s].data[0];
                (void)x;
            }
        }
        off += len;
    }

    for (int s = 0; s < 16; s++)
        if (rc[s].data && rc[s].refs > 0) free(rc[s].data);
    return 0;
}
