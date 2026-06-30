#include "nova.h"
#include <stdlib.h>
#include <string.h>

int nova_pal_init(nova_machine *mc)
{
    memset(&mc->pal, 0, sizeof(mc->pal));
    const uint8_t *p; uint32_t n;
    if (nova_cart_find(&mc->cart, TAG_PAL, &p, &n) != 0) return 0;
    if (n < 4) return 0;

    int nbanks = p[0] | (p[1] << 8);
    int ncolors = p[2] | (p[3] << 8);
    if (nbanks <= 0 || ncolors <= 0 || nbanks > 64 || ncolors > 256) return 0;

    uint32_t need = (uint32_t)nbanks * (uint32_t)ncolors * 4u;
    if (4u + need > n) return 0;

    mc->pal.entries = (uint32_t*)malloc((size_t)nbanks * ncolors * sizeof(uint32_t));
    mc->pal.nbanks = nbanks;
    mc->pal.ncolors = ncolors;
    for (int i = 0; i < nbanks * ncolors; i++) {
        const uint8_t *e = p + 4 + i * 4;
        mc->pal.entries[i] = (uint32_t)e[0] | ((uint32_t)e[1]<<8) | ((uint32_t)e[2]<<16) | ((uint32_t)e[3]<<24);
    }
    return 0;
}

void nova_pal_free(nova_machine *mc)
{
    free(mc->pal.entries);
    free(mc->pal.fade);
    mc->pal.entries = NULL;
    mc->pal.fade = NULL;
}

void nova_pal_set(nova_machine *mc, int bank)
{
    if (bank >= 0 && bank < mc->pal.nbanks)
        mc->pal.cur_bank = bank;
}

void nova_pal_cycle(nova_machine *mc, int lo, int hi)
{
    if (lo < 0) lo = 0;
    if (hi >= mc->pal.ncolors) hi = mc->pal.ncolors - 1;
    mc->pal.cyc_lo = lo;
    mc->pal.cyc_hi = hi;
    mc->pal.cyc_active = (hi > lo);
    mc->pal.cyc_phase = 0;

    int range = hi - lo + 1;
    if (range > 0 && !mc->pal.fade) {
        mc->pal.fade = (uint32_t*)malloc((size_t)range * sizeof(uint32_t));
        mc->pal.fade_cap = range;
    }
}

void nova_pal_step(nova_machine *mc)
{
    nova_palette *pl = &mc->pal;
    if (!pl->cyc_active || !pl->entries || !pl->fade) return;
    pl->cyc_phase++;
    int base = pl->cur_bank * pl->ncolors;
    for (int i = 0; i <= pl->cyc_hi - pl->cyc_lo; i++)
        pl->fade[i] = pl->entries[base + pl->cyc_lo + i];
}
