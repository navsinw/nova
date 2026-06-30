#include "nova.h"
#include <string.h>

/* a tiny finite state machine for entity AI: a transition table indexed by
   (state, event) plus a per-state dwell timer. */

void nova_fsm_init(nova_fsm *f, int nstates)
{
    memset(f, 0, sizeof(*f));
    if (nstates < 1) nstates = 1;
    if (nstates > NOVA_FSM_STATES) nstates = NOVA_FSM_STATES;
    f->nstates = nstates;
    f->state = 0;
    f->timer = 0;
    for (int s = 0; s < NOVA_FSM_STATES; s++)
        for (int e = 0; e < 4; e++)
            f->trans[s][e] = s;   /* default: stay */
}

void nova_fsm_set_trans(nova_fsm *f, int from, int event, int to)
{
    if (from < 0 || from >= f->nstates) return;
    if (event < 0 || event >= 4) return;
    if (to < 0 || to >= f->nstates) return;
    f->trans[from][event] = to;
}

int nova_fsm_fire(nova_fsm *f, int event)
{
    if (event < 0 || event >= 4) return f->state;
    int next = f->trans[f->state][event];
    if (next != f->state) {
        f->state = next;
        f->timer = 0;
    }
    return f->state;
}

void nova_fsm_tick(nova_fsm *f)
{
    f->timer++;
}
