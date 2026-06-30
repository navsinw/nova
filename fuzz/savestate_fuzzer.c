#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "nova.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    nova_machine mc;
    memset(&mc, 0, sizeof(mc));
    nova_savestate_load(&mc, data, (uint32_t)size);
    return 0;
}
