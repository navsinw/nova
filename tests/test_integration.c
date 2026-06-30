#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

/* cart with CODE[halt] + FONT (8 simple 4x4 glyphs) so text rendering works. */
static int build_cart(uint8_t *buf)
{
    memset(buf, 0, 512);
    memcpy(buf, "NOVA", 4); buf[4] = 2; buf[8] = 2;     /* CODE + FONT */
    int off = 18 + 24;
    buf[18]='C';buf[19]='O';buf[20]='D';buf[21]='E'; buf[22]=(uint8_t)off; buf[26]=1; buf[off]=OP_HALT;
    int total = off + 1;
    int foff = total;
    buf[30]='F';buf[31]='O';buf[32]='N';buf[33]='T'; buf[34]=(uint8_t)foff;
    int fsz = 2 + 8 * (4 + 16);
    buf[38]=(uint8_t)(fsz & 0xff); buf[39]=(uint8_t)((fsz >> 8) & 0xff);
    uint8_t *f = buf + foff;
    f[0] = 8; f[1] = 0;
    int p = 2;
    for (int g = 0; g < 8; g++) {
        f[p]=4; f[p+1]=4; f[p+2]=0; f[p+3]=0; p += 4;
        for (int i = 0; i < 16; i++) f[p++] = (uint8_t)((i + g) & 1);
    }
    return foff + fsz;
}

static void test_text_ui(void)
{
    uint8_t buf[512];
    int total = build_cart(buf);
    nova_machine mc;
    CHECK(nova_machine_load(&mc, buf, (size_t)total) == 0);
    CHECK(mc.font.nglyphs == 8);

    nova_raster_clear(&mc, 0);
    char str[5] = { 1, 2, 3, 1, 0 };
    int drawn = nova_text_draw(&mc, str, 10, 10, 7);
    CHECK(drawn == 4);
    nova_font_blit(&mc, 1, 0, 0, 5);

    nova_ui_panel(&mc, 4, 4, 60, 30, 6, 15);
    nova_ui_button(&mc, 8, 8, 40, 10, str, 1);
    nova_ui_label(&mc, 8, 22, str, 9);
    nova_ui_bar(&mc, 8, 30, 40, 6, 3, 10, 11);
    nova_ui_progress(&mc, 8, 40, 50, 128);

    /* something got drawn into the framebuffer */
    int nonzero = 0;
    for (int i = 0; i < NOVA_FB_SIZE; i++) if (mc.gfx.fb[i]) nonzero++;
    CHECK(nonzero > 0);

    char pgm[4096];
    int pn = nova_fb_to_pgm(mc.gfx.fb, 16, 16, pgm, sizeof(pgm));
    CHECK(pn > 0);

    nova_machine_free(&mc);
}

static void test_softsynth_wavetable(void)
{
    int16_t tab[256];
    CHECK(nova_wavetable_gen(1, tab, 256) == 256);   /* square */
    int nonzero = 0;
    for (int i = 0; i < 256; i++) if (tab[i]) nonzero++;
    CHECK(nonzero > 0);
    int16_t a[64], b[64], mix[64];
    nova_wavetable_gen(0, a, 64);
    nova_wavetable_gen(2, b, 64);
    CHECK(nova_wavetable_mix(a, b, mix, 64, 128) == 64);

    nova_softsynth ss;
    nova_softsynth_init(&ss);
    nova_softsynth_noteon(&ss, 0, 60, 0);
    nova_softsynth_noteon(&ss, 1, 64, 1);
    int16_t out[256];
    int rendered = 0;
    for (int frame = 0; frame < 4; frame++)
        rendered += nova_softsynth_render(&ss, out, 256);
    CHECK(rendered == 4 * 256);
    int any = 0;
    for (int i = 0; i < 256; i++) if (out[i]) any = 1;
    CHECK(any);
    nova_softsynth_noteoff(&ss, 0);
    nova_softsynth_noteoff(&ss, 1);
    for (int frame = 0; frame < 64; frame++) nova_softsynth_render(&ss, out, 256);
    CHECK(!ss.voices[0].active);
}

static void test_blend_minimap(void)
{
    uint8_t buf[512];
    int total = build_cart(buf);
    nova_machine mc;
    CHECK(nova_machine_load(&mc, buf, (size_t)total) == 0);

    uint8_t *layer = (uint8_t*)malloc(NOVA_FB_SIZE);
    memset(layer, 3, NOVA_FB_SIZE);
    nova_raster_clear(&mc, 0);
    nova_fb_blend(mc.gfx.fb, layer, NOVA_FB_SIZE, NOVA_BLEND_OR);
    CHECK(mc.gfx.fb[100] == 3);
    nova_fb_fade(mc.gfx.fb, NOVA_FB_SIZE, 1);
    CHECK(mc.gfx.fb[100] == 2);
    uint8_t *mask = (uint8_t*)calloc(NOVA_FB_SIZE, 1);
    mask[50] = 1;
    uint8_t *src = (uint8_t*)malloc(NOVA_FB_SIZE);
    memset(src, 9, NOVA_FB_SIZE);
    nova_fb_mask(mc.gfx.fb, src, mask, NOVA_FB_SIZE);
    CHECK(mc.gfx.fb[50] == 9);
    free(layer); free(mask); free(src);

    uint16_t tiles[8 * 8];
    for (int i = 0; i < 64; i++) tiles[i] = (uint16_t)(i & 7);
    nova_minimap_draw(&mc, tiles, 8, 8, 0, 0, 2);

    nova_machine_free(&mc);
}

int main(void)
{
    test_text_ui();
    test_softsynth_wavetable();
    test_blend_minimap();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
