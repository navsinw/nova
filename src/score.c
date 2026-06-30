#include "nova.h"

/* score tracking with a combo multiplier and a 128-bit high-score record. */

void nova_score_init(nova_score *s)
{
    nova_u128_set(&s->value, 0);
    nova_u128_set(&s->high, 0);
    s->multiplier = 1;
}

void nova_score_add(nova_score *s, uint32_t points)
{
    nova_u128 add;
    nova_u128_set(&add, points);
    if (s->multiplier > 1) nova_u128_mul_u32(&add, (uint32_t)s->multiplier);
    nova_u128_add(&s->value, &add);
}

void nova_score_combo(nova_score *s, int mult)
{
    if (mult < 1) mult = 1;
    if (mult > 999) mult = 999;
    s->multiplier = mult;
}

int nova_score_commit_high(nova_score *s)
{
    if (nova_u128_cmp(&s->value, &s->high) > 0) {
        s->high = s->value;
        return 1;
    }
    return 0;
}

int nova_score_str(nova_score *s, char *out, int outcap)
{
    return nova_u128_to_dec(&s->value, out, outcap);
}
