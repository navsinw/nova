#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "nova.h"

/* drives the display-list coprocessor (and the rasterizer it calls) over a
   machine that has a couple of sprite banks loaded. */

static int build_host(uint8_t *buf)
{
    memset(buf, 0, 256);
    memcpy(buf, "NOVA", 4);
    buf[4] = 2;
    buf[8] = 2;                 /* CODE + SPRT */
    int off = 18 + 24;
    buf[18]='C';buf[19]='O';buf[20]='D';buf[21]='E';
    buf[22]=(uint8_t)off; buf[26]=1;
    buf[off]=OP_HALT;
    int total = off + 1;
    int soff = total;
    buf[30]='S';buf[31]='P';buf[32]='R';buf[33]='T';
    buf[34]=(uint8_t)soff;
    int sprt = 8 + 2*(4+64);
    buf[38]=(uint8_t)(sprt & 0xff); buf[39]=(uint8_t)((sprt>>8)&0xff);
    uint8_t *s = buf + soff;
    s[0]=2; s[2]=2; s[4]=4;
    int p = 8;
    for (int b=0;b<2;b++){ s[p]=8; s[p+2]=8; p+=4; for(int i=0;i<64;i++) s[p++]=(uint8_t)i; }
    return soff + sprt;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t host[256];
    int total = build_host(host);
    nova_machine mc;
    if (nova_machine_load(&mc, host, (size_t)total) != 0)
        return 0;
    nova_gpu_run(&mc, data, (uint32_t)size);
    nova_machine_free(&mc);
    return 0;
}
