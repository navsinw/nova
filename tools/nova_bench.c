#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* nova_bench: load a cart and run it many times, reporting throughput. */

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

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s cart.nova [iters] [ticks]\n", argv[0]); return 2; }
    int iters = argc >= 3 ? atoi(argv[2]) : 1000;
    int ticks = argc >= 4 ? atoi(argv[3]) : 60;
    if (iters < 1) iters = 1; if (iters > 1000000) iters = 1000000;
    if (ticks < 0) ticks = 0; if (ticks > 100000) ticks = 100000;

    size_t len = 0;
    uint8_t *data = read_file(argv[1], &len);
    if (!data) { fprintf(stderr, "cannot read\n"); return 1; }

    /* sanity load once */
    nova_machine probe;
    if (nova_machine_load(&probe, data, len) != 0) { fprintf(stderr, "load failed\n"); free(data); return 1; }
    nova_machine_free(&probe);

    clock_t t0 = clock();
    long total_ticks = 0;
    for (int i = 0; i < iters; i++) {
        nova_machine mc;
        if (nova_machine_load(&mc, data, len) != 0) break;
        total_ticks += nova_machine_run(&mc, ticks);
        nova_machine_free(&mc);
    }
    clock_t t1 = clock();
    double secs = (double)(t1 - t0) / CLOCKS_PER_SEC;
    if (secs <= 0) secs = 1e-6;
    printf("iters=%d ticks/iter=%d total_ticks=%ld\n", iters, ticks, total_ticks);
    printf("elapsed=%.3fs  %.0f loads/s\n", secs, iters / secs);
    free(data);
    return 0;
}
