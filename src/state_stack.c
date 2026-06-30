#include "nova.h"

/* a bounded stack of game states (title/play/pause/gameover). */

void nova_sstack_init(nova_sstack *s)
{
    s->top = 0;
}

int nova_sstack_push(nova_sstack *s, int state)
{
    if (s->top >= NOVA_STATE_MAX) return -1;
    s->states[s->top++] = state;
    return 0;
}

int nova_sstack_pop(nova_sstack *s)
{
    if (s->top <= 0) return -1;
    return s->states[--s->top];
}

int nova_sstack_top(const nova_sstack *s)
{
    if (s->top <= 0) return -1;
    return s->states[s->top - 1];
}
