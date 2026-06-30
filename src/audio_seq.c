#include "nova.h"

/* a step arpeggiator: cycles through pitch offsets from a root note. */

void nova_arp_init(nova_arp *a, int root)
{
    a->root = root;
    a->n = 0;
    a->pos = 0;
    a->gate = 1;
}

int nova_arp_set(nova_arp *a, const int *offs, int n)
{
    if (n < 0) n = 0;
    if (n > NOVA_ARP_STEPS) n = NOVA_ARP_STEPS;
    a->n = n;
    for (int i = 0; i < n; i++) a->steps[i] = offs ? offs[i] : 0;
    a->pos = 0;
    return n;
}

int nova_arp_next(nova_arp *a)
{
    if (a->n <= 0) return a->root;
    int note = a->root + a->steps[a->pos % a->n];
    a->pos = (a->pos + 1) % a->n;
    if (note < 0) note = 0;
    if (note > 119) note = 119;
    return note;
}
