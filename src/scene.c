#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* a small moving-entity layer: entities carry velocity and an animation,
   bounce off the play-field bounds, and draw through the sprite path. */

void nova_scene_init(nova_scene *s, int w, int h)
{
    s->cap = NOVA_SCENE_MAX;
    s->ents = (nova_entity*)calloc(s->cap, sizeof(nova_entity));
    s->n = 0;
    s->bounds_w = w > 0 ? w : NOVA_FB_W;
    s->bounds_h = h > 0 ? h : NOVA_FB_H;
}

int nova_scene_spawn(nova_scene *s, int x, int y, int sprite)
{
    if (!s->ents || s->n >= s->cap) return -1;
    nova_entity *e = &s->ents[s->n];
    memset(e, 0, sizeof(*e));
    e->x = x; e->y = y;
    e->vx = 1; e->vy = 1;
    e->sprite = sprite;
    e->alive = 1;
    e->hp = 3;
    int frames[4] = { sprite, sprite + 1, sprite + 2, sprite + 1 };
    nova_anim_init(&e->anim, frames, 4, 4, 1);
    return s->n++;
}

void nova_scene_update(nova_scene *s)
{
    if (!s->ents) return;
    for (int i = 0; i < s->n; i++) {
        nova_entity *e = &s->ents[i];
        if (!e->alive) continue;
        e->x += e->vx;
        e->y += e->vy;
        if (e->x < 0) { e->x = 0; e->vx = -e->vx; }
        if (e->y < 0) { e->y = 0; e->vy = -e->vy; }
        if (e->x >= s->bounds_w) { e->x = s->bounds_w - 1; e->vx = -e->vx; }
        if (e->y >= s->bounds_h) { e->y = s->bounds_h - 1; e->vy = -e->vy; }
        nova_anim_step(&e->anim);
    }
}

void nova_scene_draw(nova_machine *mc, nova_scene *s)
{
    if (!s->ents) return;
    for (int i = 0; i < s->n; i++) {
        nova_entity *e = &s->ents[i];
        if (!e->alive) continue;
        nova_gfx_sprite(mc, nova_anim_frame(&e->anim), e->x, e->y);
    }
}

void nova_scene_free(nova_scene *s)
{
    free(s->ents);
    s->ents = NULL;
    s->n = s->cap = 0;
}
