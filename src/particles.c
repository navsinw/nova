#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* a fixed-capacity particle pool with a gravity field, drawn as raster pixels. */

void nova_psys_init(nova_psys *ps, uint32_t seed)
{
    ps->cap = NOVA_MAX_PARTICLES;
    ps->p = (nova_particle*)calloc(ps->cap, sizeof(nova_particle));
    ps->n = 0;
    ps->gravity = 1;
    nova_rng_seed(&ps->rng, seed);
}

static int alloc_slot(nova_psys *ps)
{
    for (int i = 0; i < ps->cap; i++)
        if (!ps->p[i].active) { if (i + 1 > ps->n) ps->n = i + 1; return i; }
    return -1;
}

void nova_psys_emit(nova_psys *ps, int x, int y, int count, int color)
{
    if (!ps->p) return;
    for (int k = 0; k < count; k++) {
        int s = alloc_slot(ps);
        if (s < 0) break;
        nova_particle *pt = &ps->p[s];
        pt->x = x;
        pt->y = y;
        pt->vx = nova_rng_range(&ps->rng, -3, 4);
        pt->vy = nova_rng_range(&ps->rng, -5, 0);
        pt->life = nova_rng_range(&ps->rng, 16, 48);
        pt->color = color;
        pt->active = 1;
    }
}

void nova_psys_update(nova_psys *ps)
{
    if (!ps->p) return;
    for (int i = 0; i < ps->n; i++) {
        nova_particle *pt = &ps->p[i];
        if (!pt->active) continue;
        pt->x += pt->vx;
        pt->y += pt->vy;
        pt->vy += ps->gravity;
        pt->life--;
        if (pt->life <= 0) pt->active = 0;
    }
}

void nova_psys_draw(nova_machine *mc, nova_psys *ps)
{
    if (!ps->p) return;
    for (int i = 0; i < ps->n; i++) {
        nova_particle *pt = &ps->p[i];
        if (!pt->active) continue;
        nova_raster_pixel(mc, pt->x, pt->y, (uint8_t)pt->color);
    }
}

void nova_psys_free(nova_psys *ps)
{
    free(ps->p);
    ps->p = NULL;
    ps->n = ps->cap = 0;
}
