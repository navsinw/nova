#include <stdint.h>
#include <stddef.h>
#include "nova.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    nova_machine mc;
    if (nova_machine_load(&mc, data, size) != 0)
        return 0;
    nova_tracker_play(&mc, 0);
    for (int i = 0; i < 256; i++)
        nova_machine_tick(&mc);
    nova_machine_free(&mc);
    return 0;
}
