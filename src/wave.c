#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* PCM sample chunk: [u32 nsamples][u16 rate] then nsamples int16 little-endian.
   used for one-shot sound effects mixed alongside the synth. */

int nova_wave_load(nova_wave *w, const uint8_t *data, uint32_t len)
{
    memset(w, 0, sizeof(*w));
    if (!data || len < 6) return -1;
    uint32_t ns = (uint32_t)data[0] | ((uint32_t)data[1]<<8) | ((uint32_t)data[2]<<16) | ((uint32_t)data[3]<<24);
    int rate = data[4] | (data[5] << 8);
    if (ns == 0 || ns > (1u << 22)) return -1;
    if (6u + ns * 2u > len) ns = (len - 6u) / 2u;
    if (ns == 0) return -1;
    w->samples = (int16_t*)malloc((size_t)ns * sizeof(int16_t));
    if (!w->samples) return -1;
    for (uint32_t i = 0; i < ns; i++)
        w->samples[i] = (int16_t)(data[6 + i*2] | (data[6 + i*2 + 1] << 8));
    w->nsamples = (int)ns;
    w->rate = rate ? rate : NOVA_SAMPLE_RATE;
    w->pos = 0;
    w->playing = 1;
    return 0;
}

int nova_wave_mix(nova_wave *w, int16_t *out, int n)
{
    if (!w->playing || !w->samples) return 0;
    int mixed = 0;
    for (int i = 0; i < n; i++) {
        if (w->pos >= w->nsamples) { w->playing = 0; break; }
        int v = out[i] + (w->samples[w->pos] >> 1);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        out[i] = (int16_t)v;
        w->pos++;
        mixed++;
    }
    return mixed;
}

void nova_wave_free(nova_wave *w)
{
    free(w->samples);
    w->samples = NULL;
    w->nsamples = 0;
    w->playing = 0;
}
