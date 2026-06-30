#include "nova.h"
#include <stdio.h>
#include <stdlib.h>

/* nova_view: run a cart and dump the framebuffer as ASCII art or a PPM. */

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    uint8_t *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    *len = fread(buf, 1, (size_t)n, f); fclose(f);
    return buf;
}

static const char ramp[] = " .:-=+*#%@";

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s cart.nova [ticks] [--ppm]\n", argv[0]); return 2; }
    int ticks = argc >= 3 ? atoi(argv[2]) : 30;
    int ppm = (argc >= 4 && argv[3][0] == '-');
    if (ticks < 0) ticks = 0; if (ticks > 100000) ticks = 100000;

    size_t len = 0;
    uint8_t *data = read_file(argv[1], &len);
    if (!data) { fprintf(stderr, "cannot read\n"); return 1; }

    nova_machine mc;
    if (nova_machine_load(&mc, data, len) != 0) { fprintf(stderr, "load failed\n"); free(data); return 1; }
    nova_machine_run(&mc, ticks);

    if (!mc.gfx.fb) { fprintf(stderr, "no framebuffer\n"); nova_machine_free(&mc); free(data); return 1; }

    if (ppm) {
        printf("P2\n%d %d\n255\n", NOVA_FB_W, NOVA_FB_H);
        for (int i = 0; i < NOVA_FB_SIZE; i++)
            printf("%d%c", mc.gfx.fb[i], (i % NOVA_FB_W == NOVA_FB_W - 1) ? '\n' : ' ');
    } else {
        for (int y = 0; y < NOVA_FB_H; y += 4) {
            for (int x = 0; x < NOVA_FB_W; x += 2) {
                int v = mc.gfx.fb[y * NOVA_FB_W + x];
                putchar(ramp[(v * 9) / 255]);
            }
            putchar('\n');
        }
    }
    nova_machine_free(&mc);
    free(data);
    return 0;
}
