#include "nova.h"
#include <string.h>

/* a typewriter-style dialogue box: reveals one character every `speed` ticks. */

void nova_dialogue_set(nova_dialogue *d, const char *text, int speed)
{
    int n = 0;
    while (text && text[n] && n < (int)sizeof(d->text) - 1) { d->text[n] = text[n]; n++; }
    d->text[n] = 0;
    d->len = n;
    d->pos = 0;
    d->speed = speed > 0 ? speed : 1;
    d->timer = 0;
    d->done = (n == 0);
}

void nova_dialogue_advance(nova_dialogue *d)
{
    if (d->done) return;
    d->timer++;
    if (d->timer < d->speed) return;
    d->timer = 0;
    if (d->pos < d->len) d->pos++;
    if (d->pos >= d->len) d->done = 1;
}

int nova_dialogue_visible(nova_dialogue *d, char *out, int outcap)
{
    int n = d->pos;
    if (n > outcap - 1) n = outcap - 1;
    memcpy(out, d->text, (size_t)n);
    out[n] = 0;
    return n;
}

void nova_dialogue_skip(nova_dialogue *d)
{
    d->pos = d->len;
    d->done = 1;
}
