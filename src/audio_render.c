#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* tiny software synth: per-voice oscillator bank mixed into a PCM block.
   exercised once per machine tick so the sequencer actually makes sound. */

static int   g_tables_ready;
static int16_t g_sine[256];
static int16_t g_tri[256];
static int16_t g_saw[256];
static int16_t g_square[256];

static void build_tables(void)
{
    if (g_tables_ready) return;
    for (int i = 0; i < 256; i++) {
        /* parabolic sine approximation, plenty for an 8-bit box */
        int x = i - 128;
        int s = (i < 128) ? (i * 2 - 64) : (192 - i * 2);
        int para = (16384 - x * x / 2);
        if (para > 16384) para = 16384;
        g_sine[i] = (int16_t)((para - 8192) * 3 + s * 8);
        g_tri[i]  = (int16_t)((i < 128 ? (i - 64) : (192 - i)) * 256);
        g_saw[i]  = (int16_t)((i - 128) * 256);
        g_square[i] = (int16_t)(i < 128 ? 12000 : -12000);
    }
    g_tables_ready = 1;
}

/* equal-tempered-ish period lookup keyed by midi-ish pitch */
int nova_note_period(int pitch)
{
    static const int base[12] = {
        428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226
    };
    if (pitch < 0) pitch = 0;
    if (pitch > 119) pitch = 119;
    int oct = pitch / 12;
    int note = pitch % 12;
    int p = base[note];
    for (int i = 5; i < oct; i++) p /= 2;
    for (int i = oct; i < 5; i++) p *= 2;
    if (p < 1) p = 1;
    return p;
}

static int16_t osc_sample(int wave, uint32_t phase, uint32_t *lfsr)
{
    uint8_t idx = (uint8_t)(phase >> 24);
    switch (wave & 3) {
    case 0: return g_sine[idx];
    case 1: return g_tri[idx];
    case 2: return g_saw[idx];
    default: {
        uint32_t x = *lfsr ? *lfsr : 0xACE1u;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        *lfsr = x;
        return (int16_t)((x & 0x3fff) - 8192);
    }
    }
}

static int voice_level(nova_machine *mc, nova_voice *v)
{
    if (v->instr < 0 || v->instr >= mc->synth.ninstrs) return 0;
    nova_instr *it = &mc->synth.instrs[v->instr];
    if (it->npts <= 0) return 0;
    int idx = v->env_idx;
    if (idx < 0) idx = 0;
    if (idx >= it->npts) idx = it->npts - 1;
    int lvl = it->pts[idx];
    if (lvl < 0) lvl = -lvl;
    if (lvl > 4096) lvl = 4096;
    return lvl;
}

void nova_audio_render(nova_machine *mc)
{
    build_tables();
    if (!mc->audio_buf) {
        mc->audio_cap = NOVA_AUDIO_BLOCK;
        mc->audio_buf = (int16_t*)calloc(mc->audio_cap, sizeof(int16_t));
        mc->noise_lfsr = 0x1234u;
    }
    int n = mc->audio_cap;
    memset(mc->audio_buf, 0, (size_t)n * sizeof(int16_t));

    for (int c = 0; c < NOVA_MAX_VOICES; c++) {
        nova_voice *v = &mc->synth.voices[c];
        if (!v->active) continue;
        int period = nova_note_period(v->pitch);
        uint32_t step = (uint32_t)((0x1000000u) / (uint32_t)(period < 1 ? 1 : period));
        int wave = (v->instr >= 0 && v->instr < mc->synth.ninstrs)
                   ? (mc->synth.instrs[v->instr].lfo_rate & 3) : 0;
        int lvl = voice_level(mc, v);
        v->cur_level = lvl;
        for (int i = 0; i < n; i++) {
            int16_t s = osc_sample(wave, v->osc_phase, &mc->noise_lfsr);
            int mixed = mc->audio_buf[i] + (s * lvl >> 13);
            if (mixed > 32767) mixed = 32767;
            if (mixed < -32768) mixed = -32768;
            mc->audio_buf[i] = (int16_t)mixed;
            v->osc_phase += step;
        }
    }

    /* gentle one-pole low pass so the block isn't pure aliasing */
    int prev = 0;
    for (int i = 0; i < n; i++) {
        int cur = mc->audio_buf[i];
        int out = (cur + prev) / 2;
        mc->audio_buf[i] = (int16_t)out;
        prev = cur;
    }
}

void nova_audio_free(nova_machine *mc)
{
    free(mc->audio_buf);
    mc->audio_buf = NULL;
    mc->audio_cap = 0;
}
