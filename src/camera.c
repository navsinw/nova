#include "nova.h"

/* a smoothed follow camera with screen shake, clamped to world bounds. */

void nova_camera_init(nova_camera *c, int bw, int bh)
{
    c->x = c->y = c->tx = c->ty = 0;
    c->shake = 0;
    c->shake_seed = 12345;
    c->bx = c->by = 0;
    c->bw = bw > 0 ? bw : NOVA_FB_W;
    c->bh = bh > 0 ? bh : NOVA_FB_H;
}

void nova_camera_follow(nova_camera *c, int tx, int ty)
{
    c->tx = tx - NOVA_FB_W / 2;
    c->ty = ty - NOVA_FB_H / 2;
}

void nova_camera_shake(nova_camera *c, int amount)
{
    if (amount > c->shake) c->shake = amount;
}

void nova_camera_update(nova_camera *c)
{
    /* approach target by a fraction */
    c->x += (c->tx - c->x) / 4;
    c->y += (c->ty - c->y) / 4;

    int maxx = c->bw - NOVA_FB_W;
    int maxy = c->bh - NOVA_FB_H;
    if (maxx < 0) maxx = 0;
    if (maxy < 0) maxy = 0;
    if (c->x < c->bx) c->x = c->bx;
    if (c->y < c->by) c->y = c->by;
    if (c->x > c->bx + maxx) c->x = c->bx + maxx;
    if (c->y > c->by + maxy) c->y = c->by + maxy;

    if (c->shake > 0) c->shake--;
}

void nova_camera_apply(nova_machine *mc, nova_camera *c)
{
    int ox = 0, oy = 0;
    if (c->shake > 0) {
        c->shake_seed = c->shake_seed * 1103515245 + 12345;
        ox = ((c->shake_seed >> 16) % (2 * c->shake + 1)) - c->shake;
        c->shake_seed = c->shake_seed * 1103515245 + 12345;
        oy = ((c->shake_seed >> 16) % (2 * c->shake + 1)) - c->shake;
    }
    mc->gfx.cam_x = c->x + ox;
    mc->gfx.cam_y = c->y + oy;
}
