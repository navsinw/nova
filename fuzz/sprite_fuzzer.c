#include <stdint.h>
#include <stddef.h>
#include "nova.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    nova_machine mc;
    if (nova_machine_load(&mc, data, size) != 0)
        return 0;
    for (int id = 0; id < 8; id++) {
        nova_gfx_sprite(&mc, id, id * 12, id * 7);
        nova_gfx_tile(&mc, 0, 0, 4, 4);
    }
    nova_pal_cycle(&mc, 0, 7);
    for (int i = 0; i < 32; i++)
        nova_pal_step(&mc);
    nova_machine_free(&mc);
    return 0;
}
