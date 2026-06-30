#include "nova.h"
#include <stdlib.h>
#include <string.h>

static uint16_t s16(const uint8_t *p){ return (uint16_t)(p[0]|(p[1]<<8)); }

int nova_synth_init(nova_machine *mc)
{
    memset(&mc->synth, 0, sizeof(mc->synth));
    mc->synth.modq = (nova_mod**)calloc(NOVA_MAX_VOICES * 2, sizeof(nova_mod*));
    const uint8_t *p; uint32_t n;
    if (nova_cart_find(&mc->cart, TAG_SND, &p, &n) != 0) return 0;
    if (n < 2) return 0;

    int ni = s16(p);
    if (ni <= 0 || ni > 256) return 0;
    mc->synth.instrs = (nova_instr*)calloc(ni, sizeof(nova_instr));
    mc->synth.ninstrs = ni;

    uint32_t off = 2;
    for (int i = 0; i < ni; i++) {
        if (off + 10 > n) { mc->synth.ninstrs = i; break; }
        int np = s16(p + off);
        int ls = (int16_t)s16(p + off + 2);
        int le = (int16_t)s16(p + off + 4);
        int sus = (int16_t)s16(p + off + 6);
        int depth = (int8_t)p[off + 8];
        int rate = (int8_t)p[off + 9];
        off += 10;
        if (np < 0 || np > 4096) { mc->synth.ninstrs = i; break; }
        if (off + (uint32_t)np * 2u > n) { mc->synth.ninstrs = i; break; }
        nova_instr *it = &mc->synth.instrs[i];
        it->pts = (int16_t*)malloc((np ? np : 1) * sizeof(int16_t));
        it->npts = np;
        it->loop_start = (ls >= 0 && ls < np) ? ls : 0;
        it->loop_end = (le > 0 && le <= np) ? le : np;
        it->sustain = sus;
        it->lfo_depth = depth;
        it->lfo_rate = rate;
        for (int k = 0; k < np; k++)
            it->pts[k] = (int16_t)s16(p + off + k*2);
        off += (uint32_t)np * 2u;
    }
    return 0;
}

void nova_synth_free(nova_machine *mc)
{
    for (int i = 0; i < mc->synth.ninstrs; i++)
        free(mc->synth.instrs[i].pts);
    free(mc->synth.instrs);
    mc->synth.instrs = NULL;
    for (int i = 0; i < mc->synth.nmods; i++)
        free(mc->synth.modq[i]);
    free(mc->synth.modq);
    mc->synth.modq = NULL;
    mc->synth.nmods = 0;
}

void nova_synth_voice(nova_machine *mc, int chan, int instr)
{
    if (chan < 0 || chan >= NOVA_MAX_VOICES) return;
    if (instr < 0 || instr >= mc->synth.ninstrs) return;
    mc->synth.voices[chan].instr = instr;
    mc->synth.voices[chan].cur_instr = &mc->synth.instrs[instr];
}

void nova_synth_note(nova_machine *mc, int chan, int pitch)
{
    if (chan < 0 || chan >= NOVA_MAX_VOICES) return;
    nova_voice *v = &mc->synth.voices[chan];
    v->active = 1;
    v->pitch = pitch;
    v->env_idx = 0;
    v->env_frac = 0;
    v->releasing = 0;
    v->lfo_phase = 0;
    v->pending = NULL;
}

void nova_synth_noteoff(nova_machine *mc, int chan)
{
    if (chan < 0 || chan >= NOVA_MAX_VOICES) return;
    mc->synth.voices[chan].releasing = 1;
}

void nova_synth_reload(nova_machine *mc)
{
    nova_synth *s = &mc->synth;
    for (int i = 0; i < s->ninstrs; i++)
        free(s->instrs[i].pts);
    free(s->instrs);
    s->instrs = NULL;
    s->ninstrs = 0;

    const uint8_t *p; uint32_t n;
    if (nova_cart_find(&mc->cart, TAG_SND, &p, &n) != 0) return;
    if (n < 2) return;
    int ni = s16(p);
    if (ni <= 0 || ni > 256) return;
    s->instrs = (nova_instr*)calloc(ni, sizeof(nova_instr));
    s->ninstrs = ni;

    uint32_t off = 2;
    for (int i = 0; i < ni; i++) {
        if (off + 10 > n) { s->ninstrs = i; break; }
        int np = s16(p + off);
        int ls = (int16_t)s16(p + off + 2);
        int le = (int16_t)s16(p + off + 4);
        int sus = (int16_t)s16(p + off + 6);
        int depth = (int8_t)p[off + 8];
        int rate = (int8_t)p[off + 9];
        off += 10;
        if (np < 0 || np > 4096) { s->ninstrs = i; break; }
        if (off + (uint32_t)np * 2u > n) { s->ninstrs = i; break; }
        nova_instr *it = &s->instrs[i];
        it->pts = (int16_t*)malloc((np ? np : 1) * sizeof(int16_t));
        it->npts = np;
        it->loop_start = (ls >= 0 && ls < np) ? ls : 0;
        it->loop_end = (le > 0 && le <= np) ? le : np;
        it->sustain = sus;
        it->lfo_depth = depth;
        it->lfo_rate = rate;
        for (int k = 0; k < np; k++)
            it->pts[k] = (int16_t)s16(p + off + k*2);
        off += (uint32_t)np * 2u;
    }
}

void nova_synth_tick(nova_machine *mc)
{
    nova_synth *s = &mc->synth;
    s->tick++;

    for (int c = 0; c < NOVA_MAX_VOICES; c++) {
        nova_voice *v = &s->voices[c];
        if (!v->active) continue;
        nova_instr *it = v->cur_instr;
        if (!it) continue;
        if (it->npts <= 0) { v->active = 0; continue; }

        int i0 = v->env_idx;
        int i1 = i0 + 1;
        if (i1 >= it->npts) i1 = it->npts - 1;
        int a = it->pts[i0];
        int b = it->pts[i1];
        v->lfo_phase += (a - b);

        v->env_idx++;
        if (!v->releasing) {
            if (v->env_idx >= it->loop_end) v->env_idx = it->loop_start;
        } else {
            if (v->env_idx >= it->npts - 1) {
                if (v->pending) { free(v->pending); }
                v->active = 0;
            }
        }
    }

    for (int c = 0; c < NOVA_MAX_VOICES; c++) {
        nova_voice *v = &s->voices[c];
        if (!v->active) continue;
        if (v->instr < 0 || v->instr >= s->ninstrs) continue;
        nova_instr *it = &s->instrs[v->instr];
        if (it->lfo_depth != 0 && v->pending == NULL) {
            nova_mod *m = (nova_mod*)malloc(sizeof(nova_mod));
            m->target = v;
            m->delta = it->lfo_depth;
            m->when = s->tick;
            v->pending = m;
            s->modq[s->nmods++] = m;
        }
    }

    for (int k = 0; k < s->nmods; k++) {
        nova_mod *m = s->modq[k];
        m->target->lfo_phase += m->delta;
    }
}
