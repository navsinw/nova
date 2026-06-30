#include "nova.h"

/* a critically-ish damped spring in 16.16, handy for smooth UI motion. */

void nova_spring_init(nova_spring *s, int32_t pos, int32_t k, int32_t damp)
{
    s->pos = pos;
    s->vel = 0;
    s->target = pos;
    s->k = k > 0 ? k : (NOVA_FP_ONE / 4);
    s->damp = damp;
    if (s->damp < 0) s->damp = 0;
    if (s->damp > NOVA_FP_ONE) s->damp = NOVA_FP_ONE;
}

void nova_spring_update(nova_spring *s)
{
    int32_t disp = s->target - s->pos;
    int32_t accel = nova_fp_mul(disp, s->k);
    s->vel += accel;
    s->vel -= nova_fp_mul(s->vel, s->damp);
    s->pos += s->vel;
}
