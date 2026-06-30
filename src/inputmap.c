#include "nova.h"

/* button bitmask with previous-frame tracking for edge detection. */

void nova_input_init(nova_input *in)
{
    in->cur = in->prev = 0;
}

void nova_input_set(nova_input *in, uint32_t mask)
{
    in->cur = mask;
}

void nova_input_update(nova_input *in)
{
    in->prev = in->cur;
}

int nova_input_pressed(const nova_input *in, int bit)
{
    uint32_t m = 1u << (bit & 31);
    return (in->cur & m) && !(in->prev & m);
}

int nova_input_held(const nova_input *in, int bit)
{
    return (in->cur & (1u << (bit & 31))) != 0;
}

int nova_input_released(const nova_input *in, int bit)
{
    uint32_t m = 1u << (bit & 31);
    return !(in->cur & m) && (in->prev & m);
}
