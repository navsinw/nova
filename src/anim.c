#include "nova.h"

/* sprite animation controllers: a frame list advanced on a tick counter. */

void nova_anim_init(nova_anim *a, const int *frames, int n, int speed, int loop)
{
    if (n > NOVA_ANIM_FRAMES) n = NOVA_ANIM_FRAMES;
    if (n < 0) n = 0;
    a->nframes = n;
    for (int i = 0; i < n; i++) a->frames[i] = frames ? frames[i] : i;
    a->speed = speed > 0 ? speed : 1;
    a->cur = 0;
    a->timer = 0;
    a->loop = loop;
    a->done = 0;
}

void nova_anim_reset(nova_anim *a)
{
    a->cur = 0;
    a->timer = 0;
    a->done = 0;
}

void nova_anim_step(nova_anim *a)
{
    if (a->nframes <= 0 || a->done) return;
    a->timer++;
    if (a->timer < a->speed) return;
    a->timer = 0;
    a->cur++;
    if (a->cur >= a->nframes) {
        if (a->loop) a->cur = 0;
        else { a->cur = a->nframes - 1; a->done = 1; }
    }
}

int nova_anim_frame(const nova_anim *a)
{
    if (a->nframes <= 0) return 0;
    int c = a->cur;
    if (c < 0) c = 0;
    if (c >= a->nframes) c = a->nframes - 1;
    return a->frames[c];
}
