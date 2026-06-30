#include "nova.h"
#include <string.h>

/* per-channel modulation effects driven once per tick from the sequencer.
   reads are clamped so this layer is always in-bounds. */

static const int vibr_table[16] = {
    0, 24, 49, 74, 97, 120, 141, 161, 180, 197, 212, 224, 235, 244, 250, 253
};

void nova_fx_reset(nova_machine *mc)
{
    memset(mc->fx, 0, sizeof(mc->fx));
}

static void clampch(int *chan)
{
    if (*chan < 0) *chan = 0;
    if (*chan >= NOVA_MAX_VOICES) *chan = NOVA_MAX_VOICES - 1;
}

void nova_fx_apply(nova_machine *mc, int chan, int cmd, int param)
{
    clampch(&chan);
    nova_fxstate *f = &mc->fx[chan];
    int hi = (param >> 4) & 0xf;
    int lo = param & 0xf;

    switch (cmd) {
    case 0x00:
        f->arp_a = hi;
        f->arp_b = lo;
        f->arp_pos = 0;
        f->active = (param != 0);
        break;
    case 0x01:
        f->porta_target = mc->synth.voices[chan].pitch + 12;
        f->porta_speed = param;
        f->active = 1;
        break;
    case 0x02:
        f->porta_target = mc->synth.voices[chan].pitch - 12;
        f->porta_speed = param;
        f->active = 1;
        break;
    case 0x04:
        f->vibr_speed = hi;
        f->vibr_depth = lo;
        f->active = (param != 0);
        break;
    case 0x0A:
        f->vol_slide = hi ? hi : -lo;
        f->active = 1;
        break;
    default:
        break;
    }
}

void nova_fx_tick(nova_machine *mc)
{
    nova_tracker *t = &mc->trk;
    if (t->playing && t->cur_buf && t->cur_buf->rows && t->cur_buf->num_rows > 0) {
        int row = t->cur_row;
        if (row < 0) row = 0;
        if (row >= t->cur_buf->num_rows) row = t->cur_buf->num_rows - 1;
        const uint8_t *r = t->cur_buf->rows + row * 4;
        nova_fx_apply(mc, 0, r[2] & 0x0f, r[3]);
    }

    for (int c = 0; c < NOVA_MAX_VOICES; c++) {
        nova_fxstate *f = &mc->fx[c];
        if (!f->active) continue;
        nova_voice *v = &mc->synth.voices[c];
        if (!v->active) continue;

        if (f->vibr_depth) {
            f->vibr_pos = (f->vibr_pos + f->vibr_speed) & 0x3f;
            int s = vibr_table[f->vibr_pos & 0x0f];
            v->lfo_phase += (s * f->vibr_depth) >> 6;
        }
        if (f->porta_speed) {
            if (v->pitch < f->porta_target) {
                v->pitch += f->porta_speed;
                if (v->pitch > f->porta_target) v->pitch = f->porta_target;
            } else if (v->pitch > f->porta_target) {
                v->pitch -= f->porta_speed;
                if (v->pitch < f->porta_target) v->pitch = f->porta_target;
            }
        }
        if (f->vol_slide) {
            f->vol += f->vol_slide;
            if (f->vol < 0) f->vol = 0;
            if (f->vol > 64) f->vol = 64;
        }
        if (f->arp_a || f->arp_b) {
            f->arp_pos = (f->arp_pos + 1) % 3;
        }
    }
}
