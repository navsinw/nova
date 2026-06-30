#include "nova.h"

/* classic autonomous-agent steering, returning a desired velocity in 16.16. */

nova_vec2 nova_steer_seek(nova_vec2 pos, nova_vec2 target, int32_t maxspeed)
{
    nova_vec2 dir = nova_v2_normalize(nova_v2_sub(target, pos));
    return nova_v2_scale(dir, maxspeed);
}

nova_vec2 nova_steer_flee(nova_vec2 pos, nova_vec2 target, int32_t maxspeed)
{
    nova_vec2 dir = nova_v2_normalize(nova_v2_sub(pos, target));
    return nova_v2_scale(dir, maxspeed);
}

nova_vec2 nova_steer_arrive(nova_vec2 pos, nova_vec2 target, int32_t maxspeed, int32_t slow_radius)
{
    nova_vec2 to = nova_v2_sub(target, pos);
    int32_t dist = nova_v2_len(to);
    nova_vec2 dir = nova_v2_normalize(to);
    int32_t speed = maxspeed;
    if (slow_radius > 0 && dist < slow_radius)
        speed = nova_fp_mul(maxspeed, nova_fp_div(dist, slow_radius));
    return nova_v2_scale(dir, speed);
}

nova_vec2 nova_steer_wander(nova_vec2 vel, nova_rng *r, int32_t jitter)
{
    int angle = (int)(nova_rng_next(r) % 256) - 128;
    nova_vec2 turned = nova_v2_rotate(vel, angle * jitter / 65536);
    return turned;
}
