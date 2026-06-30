#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* in-place post-processing for the synth's PCM block. */

void nova_sfx_echo(int16_t *buf, int n, int delay, int feedback)
{
    if (!buf || n <= 0 || delay <= 0 || delay >= n) return;
    if (feedback < 0) feedback = 0;
    if (feedback > 255) feedback = 255;
    for (int i = delay; i < n; i++) {
        int v = buf[i] + (buf[i - delay] * feedback >> 8);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        buf[i] = (int16_t)v;
    }
}

void nova_sfx_lowpass(int16_t *buf, int n, int cutoff)
{
    if (!buf || n <= 0) return;
    if (cutoff < 1) cutoff = 1;
    if (cutoff > 256) cutoff = 256;
    int prev = buf[0];
    for (int i = 0; i < n; i++) {
        int cur = buf[i];
        prev = prev + ((cur - prev) * cutoff >> 8);
        buf[i] = (int16_t)prev;
    }
}

void nova_sfx_highpass(int16_t *buf, int n, int cutoff)
{
    if (!buf || n <= 0) return;
    int16_t *tmp = (int16_t*)malloc((size_t)n * sizeof(int16_t));
    if (!tmp) return;
    memcpy(tmp, buf, (size_t)n * sizeof(int16_t));
    nova_sfx_lowpass(tmp, n, cutoff);
    for (int i = 0; i < n; i++) {
        int v = buf[i] - tmp[i];
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        buf[i] = (int16_t)v;
    }
    free(tmp);
}

void nova_sfx_gain(int16_t *buf, int n, int gain_q8)
{
    if (!buf || n <= 0) return;
    for (int i = 0; i < n; i++) {
        int v = buf[i] * gain_q8 >> 8;
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        buf[i] = (int16_t)v;
    }
}

void nova_sfx_clip(int16_t *buf, int n, int level)
{
    if (!buf || n <= 0) return;
    if (level < 1) level = 1;
    for (int i = 0; i < n; i++) {
        if (buf[i] > level) buf[i] = (int16_t)level;
        if (buf[i] < -level) buf[i] = (int16_t)(-level);
    }
}
