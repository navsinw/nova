#include "nova.h"
#include <string.h>

/* item inventory with per-slot stacking up to a configurable maximum. */

void nova_inv_init(nova_inventory *inv, int stack_max)
{
    memset(inv, 0, sizeof(*inv));
    inv->stack_max = stack_max > 0 ? stack_max : 99;
}

int nova_inv_add(nova_inventory *inv, int item, int count)
{
    if (count <= 0) return 0;
    int added = 0;
    /* fill existing stacks first */
    for (int i = 0; i < inv->n && count > 0; i++) {
        if (inv->slots[i].item == item && inv->slots[i].count < inv->stack_max) {
            int space = inv->stack_max - inv->slots[i].count;
            int take = count < space ? count : space;
            inv->slots[i].count += take;
            count -= take;
            added += take;
        }
    }
    /* new slots for the remainder */
    while (count > 0 && inv->n < NOVA_INV_SLOTS) {
        int take = count < inv->stack_max ? count : inv->stack_max;
        inv->slots[inv->n].item = item;
        inv->slots[inv->n].count = take;
        inv->n++;
        count -= take;
        added += take;
    }
    return added;
}

int nova_inv_remove(nova_inventory *inv, int item, int count)
{
    if (count <= 0) return 0;
    int removed = 0;
    for (int i = inv->n - 1; i >= 0 && count > 0; i--) {
        if (inv->slots[i].item != item) continue;
        int take = count < inv->slots[i].count ? count : inv->slots[i].count;
        inv->slots[i].count -= take;
        count -= take;
        removed += take;
        if (inv->slots[i].count == 0) {
            /* compact: move last slot into this hole */
            inv->slots[i] = inv->slots[inv->n - 1];
            inv->n--;
        }
    }
    return removed;
}

int nova_inv_count(const nova_inventory *inv, int item)
{
    int total = 0;
    for (int i = 0; i < inv->n; i++)
        if (inv->slots[i].item == item) total += inv->slots[i].count;
    return total;
}
