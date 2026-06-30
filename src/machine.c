#include "nova.h"
#include <stdlib.h>
#include <string.h>

int nova_machine_load(nova_machine *mc, const uint8_t *data, size_t size)
{
    memset(mc, 0, sizeof(*mc));

    if (nova_cart_open(&mc->cart, data, size) != 0) return -1;
    if (nova_mem_init(&mc->mem, &mc->cart) != 0) return -1;

    nova_pal_init(mc);
    nova_gfx_init(mc);
    nova_map_init(mc);
    nova_synth_init(mc);
    nova_tracker_init(mc);
    nova_font_init(mc);
    nova_world_init(mc);
    nova_fx_reset(mc);

    const uint8_t *sv; uint32_t svl;
    if (nova_cart_find(&mc->cart, TAG_SAVE, &sv, &svl) == 0 && svl > 0) {
        mc->save_region = (uint8_t*)malloc(svl);
        if (mc->save_region) { memcpy(mc->save_region, sv, svl); mc->save_size = (int)svl; }
    }

    if (nova_vm_init(&mc->vm, mc) != 0) return -1;
    return 0;
}

void nova_machine_tick(nova_machine *mc)
{
    nova_pal_step(mc);
    nova_tracker_tick(mc);
    nova_fx_tick(mc);
    nova_synth_tick(mc);
    nova_audio_render(mc);
    nova_world_step(mc);
    mc->ticks_run++;
}

int nova_machine_run(nova_machine *mc, int max_ticks)
{
    int t = 0;
    while (t < max_ticks) {
        nova_vm_run(mc, 4096);
        nova_machine_tick(mc);
        if (!mc->vm.running) break;
        t++;
    }
    return t;
}

void nova_machine_free(nova_machine *mc)
{
    if (!mc) return;
    nova_vm_free(&mc->vm);
    nova_world_free(mc);
    nova_font_free(mc);
    nova_tracker_free(mc);
    nova_synth_free(mc);
    nova_map_free(mc);
    nova_gfx_free(mc);
    nova_pal_free(mc);
    nova_mem_free(&mc->mem);
    nova_audio_free(mc);
    free(mc->save_region);
    nova_cart_close(&mc->cart);
}
