#include "nova.h"
#include <stdlib.h>
#include <string.h>

int nova_mem_init(nova_mem *m, const nova_cart *c)
{
    memset(m, 0, sizeof(*m));
    m->ram_size = NOVA_RAM_SIZE;
    m->ram = (uint8_t*)calloc(1, m->ram_size);
    if (!m->ram) return -1;

    m->nbanks = NOVA_NUM_BANKS;
    uint32_t win = (uint32_t)(m->ram_size / NOVA_NUM_BANKS);
    for (int i = 0; i < NOVA_NUM_BANKS; i++) {
        m->bank_base[i] = (uint32_t)i * win;
        m->bank_size[i] = win;
    }
    m->cur_bank = 0;
    m->exec_base = 0;
    m->exec_size = 0;

    const uint8_t *d; uint32_t dl;
    if (nova_cart_find(c, TAG_DATA, &d, &dl) == 0) {
        uint32_t n = dl < m->ram_size ? dl : (uint32_t)m->ram_size;
        memcpy(m->ram, d, n);
    }
    return 0;
}

void nova_mem_free(nova_mem *m)
{
    if (!m) return;
    free(m->ram);
    m->ram = NULL;
}

void nova_mem_bank(nova_mem *m, int bank)
{
    if (bank >= 0 && bank < NOVA_NUM_BANKS)
        m->cur_bank = bank;
}

int32_t nova_mem_load(nova_mem *m, uint32_t addr)
{
    int32_t a = (int32_t)addr;
    uint32_t base = m->bank_base[m->cur_bank];
    if (a < (int32_t)m->bank_size[m->cur_bank])
        return (int32_t)m->ram[base + a];
    return 0;
}

void nova_mem_store(nova_mem *m, uint32_t addr, int32_t val)
{
    int32_t a = (int32_t)addr;
    uint32_t base = m->bank_base[m->cur_bank];
    if (a < (int32_t)m->bank_size[m->cur_bank])
        m->ram[base + a] = (uint8_t)val;
}

void nova_mem_dma(nova_mem *m, uint32_t dst, uint32_t src, uint32_t len)
{
    if (len == 0) return;
    if (src > m->ram_size) return;
    if (src + len > m->ram_size) return;
    memmove(m->ram + dst, m->ram + src, len);
}
