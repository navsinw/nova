#include "nova.h"
#include <string.h>

/* a self-contained wavetable softsynth with a 4-stage ADSR per voice. */

enum { ENV_OFF = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

void nova_softsynth_init(nova_softsynth *s)
{
    memset(s, 0, sizeof(*s));
    for (int k = 0; k < 6; k++)
        nova_wavetable_gen(k, s->tables[k], 256);
    s->tables_ready = 1;
}

void nova_softsynth_noteon(nova_softsynth *s, int ch, int pitch, int wave)
{
    if (ch < 0 || ch >= NOVA_MAX_VOICES) return;
    nova_softvoice *v = &s->voices[ch];
    v->active = 1;
    v->wave = ((wave % 6) + 6) % 6;
    v->phase = 0;
    int period = nova_note_period(pitch);
    v->step = (uint32_t)(0x1000000u / (uint32_t)(period < 1 ? 1 : period));
    v->vol = 255;
    v->env_stage = ENV_ATTACK;
    v->env_level = 0;
    v->attack = 8;
    v->decay = 16;
    v->sustain = 160;
    v->release = 24;
}

void nova_softsynth_noteoff(nova_softsynth *s, int ch)
{
    if (ch < 0 || ch >= NOVA_MAX_VOICES) return;
    if (s->voices[ch].active)
        s->voices[ch].env_stage = ENV_RELEASE;
}

static void env_advance(nova_softvoice *v)
{
    switch (v->env_stage) {
    case ENV_ATTACK:
        v->env_level += v->attack;
        if (v->env_level >= 255) { v->env_level = 255; v->env_stage = ENV_DECAY; }
        break;
    case ENV_DECAY:
        v->env_level -= v->decay;
        if (v->env_level <= v->sustain) { v->env_level = v->sustain; v->env_stage = ENV_SUSTAIN; }
        break;
    case ENV_SUSTAIN:
        break;
    case ENV_RELEASE:
        v->env_level -= v->release;
        if (v->env_level <= 0) { v->env_level = 0; v->env_stage = ENV_OFF; v->active = 0; }
        break;
    default:
        break;
    }
}

int nova_softsynth_render(nova_softsynth *s, int16_t *out, int n)
{
    if (!out || n <= 0) return 0;
    for (int i = 0; i < n; i++) out[i] = 0;

    for (int c = 0; c < NOVA_MAX_VOICES; c++) {
        nova_softvoice *v = &s->voices[c];
        if (!v->active) continue;
        const int16_t *tab = s->tables[v->wave];
        for (int i = 0; i < n; i++) {
            uint8_t idx = (uint8_t)(v->phase >> 24);
            int sample = tab[idx];
            sample = sample * v->env_level >> 8;
            int mixed = out[i] + sample;
            if (mixed > 32767) mixed = 32767;
            if (mixed < -32768) mixed = -32768;
            out[i] = (int16_t)mixed;
            v->phase += v->step;
            if ((i & 7) == 0) env_advance(v);
        }
    }
    return n;
}
