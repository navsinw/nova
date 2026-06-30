#include "nova.h"

/* keyframe track: sorted (time,value) pairs sampled with linear interpolation. */

void nova_track_init(nova_track *t)
{
    t->nkeys = 0;
}

int nova_track_add(nova_track *t, int time, int value)
{
    if (t->nkeys >= NOVA_TRACK_KEYS) return -1;
    /* insert keeping time order */
    int i = t->nkeys - 1;
    while (i >= 0 && t->keys[i].time > time) {
        t->keys[i + 1] = t->keys[i];
        i--;
    }
    t->keys[i + 1].time = time;
    t->keys[i + 1].value = value;
    t->nkeys++;
    return 0;
}

int nova_track_sample(const nova_track *t, int time)
{
    if (t->nkeys == 0) return 0;
    if (time <= t->keys[0].time) return t->keys[0].value;
    if (time >= t->keys[t->nkeys - 1].time) return t->keys[t->nkeys - 1].value;
    for (int i = 0; i + 1 < t->nkeys; i++) {
        if (time >= t->keys[i].time && time <= t->keys[i + 1].time) {
            int span = t->keys[i + 1].time - t->keys[i].time;
            if (span <= 0) return t->keys[i].value;
            int dt = time - t->keys[i].time;
            return t->keys[i].value + (t->keys[i + 1].value - t->keys[i].value) * dt / span;
        }
    }
    return t->keys[t->nkeys - 1].value;
}
