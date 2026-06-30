#include "nova.h"
#include <stdio.h>
#include <stdlib.h>

/* nova_run: headless cartridge runner. loads a cart, runs a fixed number of
   ticks, and prints a framebuffer/audio digest. */

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t*)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    *len = fread(buf, 1, (size_t)n, f);
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s <cart.nova> [ticks]\n", argv[0]); return 2; }
    int ticks = argc >= 3 ? atoi(argv[2]) : 60;
    if (ticks < 0) ticks = 0;
    if (ticks > 100000) ticks = 100000;

    size_t len = 0;
    uint8_t *data = read_file(argv[1], &len);
    if (!data) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }

    nova_machine mc;
    if (nova_machine_load(&mc, data, len) != 0) {
        fprintf(stderr, "load failed\n"); free(data); return 1;
    }
    nova_machine_run(&mc, ticks);

    uint32_t fbdigest = 0;
    if (mc.gfx.fb)
        fbdigest = nova_crc32(mc.gfx.fb, NOVA_FB_SIZE);
    printf("ticks_run: %d\n", mc.ticks_run);
    printf("fb_crc32:  %08x\n", fbdigest);
    int actives = 0;
    for (int i = 0; i < NOVA_MAX_VOICES; i++)
        if (mc.synth.voices[i].active) actives++;
    printf("voices:    %d active\n", actives);

    nova_machine_free(&mc);
    free(data);
    return 0;
}
