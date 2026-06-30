#include "nova.h"

/* a sprite draw queue sorted back-to-front by z before flushing. */

void nova_batch_init(nova_batch *b)
{
    b->n = 0;
}

int nova_batch_add(nova_batch *b, int id, int x, int y, int z)
{
    if (b->n >= NOVA_BATCH_MAX) return -1;
    b->cmds[b->n].id = id;
    b->cmds[b->n].x = x;
    b->cmds[b->n].y = y;
    b->cmds[b->n].z = z;
    b->n++;
    return 0;
}

static void sort_by_z(nova_sprite_cmd *c, int n)
{
    /* insertion sort: stable and fine for small batches */
    for (int i = 1; i < n; i++) {
        nova_sprite_cmd key = c[i];
        int j = i - 1;
        while (j >= 0 && c[j].z > key.z) { c[j+1] = c[j]; j--; }
        c[j+1] = key;
    }
}

void nova_batch_flush(nova_machine *mc, nova_batch *b)
{
    sort_by_z(b->cmds, b->n);
    for (int i = 0; i < b->n; i++)
        nova_gfx_sprite(mc, b->cmds[i].id, b->cmds[i].x, b->cmds[i].y);
    b->n = 0;
}
