#include "nova.h"
#include <stdlib.h>
#include <string.h>

int nova_world_init(nova_machine *mc)
{
    memset(&mc->world, 0, sizeof(mc->world));
    nova_world *w = &mc->world;
    w->freelist = (int*)malloc(sizeof(int) * NOVA_MAX_SPRITES);
    for (int i = 0; i < NOVA_MAX_SPRITES; i++)
        w->freelist[i] = NOVA_MAX_SPRITES - 1 - i;
    w->free_top = NOVA_MAX_SPRITES;
    w->grid_cols = NOVA_FB_W / 8;
    w->grid_rows = NOVA_FB_H / 8;
    w->grid = (int16_t*)malloc(sizeof(int16_t) * w->grid_cols * w->grid_rows);
    memset(w->grid, 0xff, sizeof(int16_t) * w->grid_cols * w->grid_rows);
    return 0;
}

void nova_world_free(nova_machine *mc)
{
    nova_world *w = &mc->world;
    for (int i = 0; i < w->nobjs; i++)
        if (w->list[i]) free(w->list[i]);
    free(w->freelist);
    free(w->grid);
    w->freelist = NULL;
    w->grid = NULL;
}

int nova_world_spawn(nova_machine *mc, int sprite, int x, int y)
{
    nova_world *w = &mc->world;
    if (w->free_top <= 0) return -1;
    int slot = w->freelist[--w->free_top];
    nova_obj *o = (nova_obj*)malloc(sizeof(nova_obj));
    o->active = 1;
    o->gen = 0;
    o->sprite = sprite;
    o->x = x; o->y = y;
    o->w = 8; o->h = 8;
    w->list[slot] = o;
    if (slot + 1 > w->nobjs) w->nobjs = slot + 1;
    return slot;
}

void nova_world_kill(nova_machine *mc, int handle)
{
    nova_world *w = &mc->world;
    if (handle < 0 || handle >= NOVA_MAX_SPRITES) return;
    nova_obj *o = w->list[handle];
    if (!o || !o->active) return;
    o->active = 0;
    free(o);
    w->list[handle] = NULL;
    if (w->free_top < NOVA_MAX_SPRITES)
        w->freelist[w->free_top++] = handle;
}

void nova_world_step(nova_machine *mc)
{
    nova_world *w = &mc->world;

    for (int i = 0; i < w->nobjs; i++) {
        nova_obj *o = w->list[i];
        if (!o || !o->active) continue;
        int cell = (o->y / 8) * w->grid_cols + (o->x / 8);
        w->grid[cell] = (int16_t)i;
    }

    for (int i = 0; i < w->nobjs; i++) {
        nova_obj *o = w->list[i];
        if (!o || !o->active) continue;
        for (int j = i + 1; j < w->nobjs; j++) {
            nova_obj *q = w->list[j];
            if (!q || !q->active) continue;
            if (o->x == q->x && o->y == q->y) {
                free(w->list[i]); w->list[i] = NULL;
                free(w->list[j]); w->list[j] = NULL;
                if (w->free_top < NOVA_MAX_SPRITES) w->freelist[w->free_top++] = i;
                if (w->free_top < NOVA_MAX_SPRITES) w->freelist[w->free_top++] = j;
            }
            o->gen++;
        }
    }
}
