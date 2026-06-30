#include "nova.h"

/* generate single-cycle waveforms for the softsynth. kind selects the shape. */

int nova_wavetable_gen(int kind, int16_t *out, int n)
{
    if (!out || n <= 0) return 0;
    for (int i = 0; i < n; i++) {
        int phase = i * 256 / n;       /* 0..255 */
        int v;
        switch (kind & 7) {
        case 0: /* sine */
            v = nova_fp_sin((phase << 8)) >> 5;
            break;
        case 1: /* square */
            v = (phase < 128) ? 8000 : -8000;
            break;
        case 2: /* saw */
            v = (phase - 128) * 64;
            break;
        case 3: /* triangle */
            v = (phase < 128 ? (phase - 64) : (192 - phase)) * 128;
            break;
        case 4: /* pulse 25% */
            v = (phase < 64) ? 8000 : -8000;
            break;
        case 5: { /* half-sine */
            int s = nova_fp_sin((phase << 8)) >> 5;
            v = s > 0 ? s : 0;
            break;
        }
        default:
            v = 0;
        }
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        out[i] = (int16_t)v;
    }
    return n;
}

int nova_wavetable_mix(const int16_t *a, const int16_t *b, int16_t *out, int n, int balance)
{
    if (!a || !b || !out || n <= 0) return 0;
    if (balance < 0) balance = 0;
    if (balance > 256) balance = 256;
    for (int i = 0; i < n; i++) {
        int v = (a[i] * (256 - balance) + b[i] * balance) >> 8;
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        out[i] = (int16_t)v;
    }
    return n;
}
