#include "nova.h"
#include <stdio.h>
#include <stdlib.h>

/* nova_trace: single-step a cartridge's CPU and print a disassembly trace. */

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
    if (argc < 2) { fprintf(stderr, "usage: %s cart.nova [maxsteps]\n", argv[0]); return 2; }
    int maxsteps = argc >= 3 ? atoi(argv[2]) : 256;
    if (maxsteps < 0) maxsteps = 0;
    if (maxsteps > 100000) maxsteps = 100000;

    size_t len = 0;
    uint8_t *data = read_file(argv[1], &len);
    if (!data) { fprintf(stderr, "cannot read\n"); return 1; }

    nova_machine mc;
    if (nova_machine_load(&mc, data, len) != 0) { fprintf(stderr, "load failed\n"); free(data); return 1; }

    char line[96];
    int steps = 0;
    while (mc.vm.running && steps < maxsteps) {
        if (!mc.vm.in_ram_exec && mc.vm.pc < mc.vm.code_size) {
            nova_disasm_one(mc.vm.code, mc.vm.code_size, mc.vm.pc, line, sizeof(line));
            int top = mc.vm.sp > 0 ? mc.vm.stack[mc.vm.sp - 1] : 0;
            printf("[%4d] sp=%-3zu top=%-10d %s\n", steps, mc.vm.sp, top, line);
        } else {
            printf("[%4d] (ram-exec pc=%u)\n", steps, mc.vm.pc);
        }
        nova_vm_step(&mc);
        steps++;
    }
    printf("halted after %d steps\n", steps);
    nova_machine_free(&mc);
    free(data);
    return 0;
}
