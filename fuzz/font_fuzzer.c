#include <stdint.h>
#include <stddef.h>
#include "nova.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    nova_machine mc;
    if (nova_machine_load(&mc, data, size) != 0)
        return 0;
    nova_gfx_text(&mc, 0, 4, 4);
    nova_machine_free(&mc);
    return 0;
}
