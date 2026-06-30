#include "nova.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* a tiny debug command console: parses one line into a verb + integer args
   and pokes the running machine. returns the number of args consumed, or -1. */

static int split(char *line, char tok[6][32])
{
    int n = 0;
    char *save = NULL;
    char *t = strtok_r(line, " \t", &save);
    while (t && n < 6) {
        strncpy(tok[n], t, 31);
        tok[n][31] = 0;
        n++;
        t = strtok_r(NULL, " \t", &save);
    }
    return n;
}

static int argi(char tok[6][32], int n, int i)
{
    if (i >= n) return 0;
    return (int)strtol(tok[i], NULL, 0);
}

int nova_console_exec(nova_machine *mc, const char *line)
{
    char buf[256];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    char tok[6][32];
    int n = split(buf, tok);
    if (n == 0) return 0;

    if (strcmp(tok[0], "spr") == 0) {
        nova_gfx_sprite(mc, argi(tok,n,1), argi(tok,n,2), argi(tok,n,3));
        return 3;
    }
    if (strcmp(tok[0], "rect") == 0) {
        nova_raster_rect(mc, argi(tok,n,1), argi(tok,n,2), argi(tok,n,3), argi(tok,n,4), (uint8_t)argi(tok,n,5));
        return 5;
    }
    if (strcmp(tok[0], "fill") == 0) {
        nova_raster_fill(mc, argi(tok,n,1), argi(tok,n,2), argi(tok,n,3), argi(tok,n,4), (uint8_t)argi(tok,n,5));
        return 5;
    }
    if (strcmp(tok[0], "line") == 0) {
        nova_raster_line(mc, argi(tok,n,1), argi(tok,n,2), argi(tok,n,3), argi(tok,n,4), (uint8_t)argi(tok,n,5));
        return 5;
    }
    if (strcmp(tok[0], "circle") == 0) {
        nova_raster_circle(mc, argi(tok,n,1), argi(tok,n,2), argi(tok,n,3), (uint8_t)argi(tok,n,4));
        return 4;
    }
    if (strcmp(tok[0], "clear") == 0) {
        nova_raster_clear(mc, (uint8_t)argi(tok,n,1));
        return 1;
    }
    if (strcmp(tok[0], "cam") == 0) {
        mc->gfx.cam_x = argi(tok,n,1);
        mc->gfx.cam_y = argi(tok,n,2);
        return 2;
    }
    if (strcmp(tok[0], "note") == 0) {
        nova_synth_voice(mc, argi(tok,n,1), argi(tok,n,2));
        nova_synth_note(mc, argi(tok,n,1), argi(tok,n,3));
        return 3;
    }
    if (strcmp(tok[0], "tick") == 0) {
        int cnt = n > 1 ? argi(tok,n,1) : 1;
        if (cnt < 0) cnt = 0;
        if (cnt > 4096) cnt = 4096;
        for (int i = 0; i < cnt; i++) nova_machine_tick(mc);
        return 1;
    }
    if (strcmp(tok[0], "poke") == 0) {
        nova_mem_store(&mc->mem, (uint32_t)argi(tok,n,1), argi(tok,n,2));
        return 2;
    }
    return -1;
}
