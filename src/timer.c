#include "nova.h"

/* countdown/repeat timers: nova_timer_update returns 1 on the tick it fires. */

void nova_timer_set(nova_timer *tm, int period)
{
    tm->period = period > 0 ? period : 1;
    tm->t = tm->period;
    tm->active = 1;
    tm->fired = 0;
}

int nova_timer_update(nova_timer *tm)
{
    if (!tm->active) return 0;
    tm->t--;
    if (tm->t <= 0) {
        tm->t = tm->period;
        tm->fired++;
        return 1;
    }
    return 0;
}

void nova_timer_stop(nova_timer *tm)
{
    tm->active = 0;
}
