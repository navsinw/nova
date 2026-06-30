#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static int build_min_cart(uint8_t *buf)
{
    memset(buf, 0, 64);
    memcpy(buf, "NOVA", 4);
    buf[4] = 2; buf[8] = 1;
    uint32_t off = 18 + 12;
    buf[18]='C';buf[19]='O';buf[20]='D';buf[21]='E';
    buf[22]=(uint8_t)off; buf[26]=1; buf[off]=OP_HALT;
    return (int)off + 1;
}

static void test_hashmap(void)
{
    nova_hashmap h;
    CHECK(nova_hm_init(&h, 8) == 0);
    for (int i = 0; i < 200; i++) {
        char k[16]; snprintf(k, sizeof(k), "k%d", i);
        nova_hm_put(&h, k, i * 3);
    }
    int v = 0;
    CHECK(nova_hm_get(&h, "k42", &v) == 0 && v == 126);
    CHECK(nova_hm_get(&h, "missing", &v) != 0);
    nova_hm_put(&h, "k42", 999);
    CHECK(nova_hm_get(&h, "k42", &v) == 0 && v == 999);
    CHECK(nova_hm_del(&h, "k42") == 0);
    CHECK(nova_hm_get(&h, "k42", &v) != 0);
    CHECK(nova_hm_get(&h, "k43", &v) == 0 && v == 129);
    nova_hm_free(&h);
}

static void test_ring(void)
{
    nova_ring r;
    CHECK(nova_ring_init(&r, 4) == 0);
    CHECK(nova_ring_push(&r, 1) == 0);
    CHECK(nova_ring_push(&r, 2) == 0);
    CHECK(nova_ring_push(&r, 3) == 0);
    CHECK(nova_ring_push(&r, 4) == 0);
    CHECK(nova_ring_push(&r, 5) != 0);  /* full at cap=4 */
    uint8_t o;
    CHECK(nova_ring_pop(&r, &o) == 0 && o == 1);
    CHECK(nova_ring_push(&r, 5) == 0);
    CHECK(nova_ring_count(&r) == 4);
    nova_ring_free(&r);
}

static void test_bitset(void)
{
    nova_bitset b;
    CHECK(nova_bitset_init(&b, 100) == 0);
    nova_bitset_set(&b, 5);
    nova_bitset_set(&b, 63);
    nova_bitset_set(&b, 99);
    CHECK(nova_bitset_test(&b, 5));
    CHECK(!nova_bitset_test(&b, 6));
    CHECK(nova_bitset_count(&b) == 3);
    nova_bitset_clear(&b, 5);
    CHECK(!nova_bitset_test(&b, 5));
    CHECK(nova_bitset_count(&b) == 2);
    nova_bitset_set(&b, 1000);  /* out of range, ignored */
    CHECK(nova_bitset_count(&b) == 2);
    nova_bitset_free(&b);
}

static void test_tween(void)
{
    CHECK(nova_ease_linear(0) == 0);
    CHECK(nova_ease_linear(NOVA_FP_ONE) == NOVA_FP_ONE);
    CHECK(nova_ease_in_quad(NOVA_FP_ONE) == NOVA_FP_ONE);
    CHECK(nova_ease_out_quad(0) == 0);
    int32_t mid = nova_tween(0, 100 * NOVA_FP_ONE, NOVA_FP_ONE / 2, 0);
    CHECK(mid > 49 * NOVA_FP_ONE && mid < 51 * NOVA_FP_ONE);
    int32_t b = nova_ease_out_bounce(NOVA_FP_ONE);
    CHECK(b > NOVA_FP_ONE - 256 && b <= NOVA_FP_ONE + 256);
}

static void test_palette_ops(void)
{
    uint32_t a = 0xff000000u, b = 0xffffffffu;
    uint32_t m = nova_rgb_blend(a, b, 128);
    int r = (m >> 16) & 0xff;
    CHECK(r > 100 && r < 160);
    uint32_t s = nova_rgb_scale(0xff808080u, 128);
    CHECK(((s >> 16) & 0xff) == 64);
    uint32_t pal[8];
    nova_pal_gradient(pal, 8, a, b);
    CHECK(pal[0] == a);
    nova_pal_rotate(pal, 8, 1);
    CHECK(pal[1] == a);
}

static void test_quadtree(void)
{
    nova_quadtree q;
    CHECK(nova_qt_init(&q, 0, 0, 256, 256, 6) == 0);
    for (int i = 0; i < 100; i++)
        nova_qt_insert(&q, (i * 7) % 256, (i * 13) % 256, i);
    CHECK(q.count == 100);
    int out[128];
    int found = nova_qt_query(&q, 0, 0, 64, 64, out, 128);
    CHECK(found > 0 && found <= 100);
    nova_qt_free(&q);
}

static void test_fsm(void)
{
    nova_fsm f;
    nova_fsm_init(&f, 3);
    nova_fsm_set_trans(&f, 0, 0, 1);
    nova_fsm_set_trans(&f, 1, 1, 2);
    nova_fsm_set_trans(&f, 2, 0, 0);
    CHECK(f.state == 0);
    CHECK(nova_fsm_fire(&f, 0) == 1);
    nova_fsm_tick(&f);
    CHECK(f.timer == 1);
    CHECK(nova_fsm_fire(&f, 1) == 2);
    CHECK(f.timer == 0);  /* reset on transition */
    CHECK(nova_fsm_fire(&f, 0) == 0);
}

static void test_config(void)
{
    nova_config c;
    nova_config_init(&c);
    const char *ini = "; comment\nwidth = 160\n[audio]\nrate = 22050\nvol=8\n";
    int n = nova_config_parse(&c, ini);
    CHECK(n == 3);
    CHECK(nova_config_get_int(&c, "width", 0) == 160);
    CHECK(nova_config_get_int(&c, "audio.rate", 0) == 22050);
    CHECK(nova_config_get_int(&c, "audio.vol", 0) == 8);
    CHECK(nova_config_get(&c, "nope") == NULL);
    nova_config_free(&c);
}

static void test_soundfx(void)
{
    int16_t buf[64];
    for (int i = 0; i < 64; i++) buf[i] = (int16_t)(i * 100 - 3200);
    nova_sfx_gain(buf, 64, 128);
    CHECK(buf[63] != 0);
    nova_sfx_lowpass(buf, 64, 64);
    nova_sfx_highpass(buf, 64, 64);
    nova_sfx_echo(buf, 64, 8, 128);
    nova_sfx_clip(buf, 64, 1000);
    for (int i = 0; i < 64; i++) CHECK(buf[i] >= -1000 && buf[i] <= 1000);
}

static void test_dither(void)
{
    uint8_t fb[64];
    for (int i = 0; i < 64; i++) fb[i] = (uint8_t)(i * 4);
    nova_dither_ordered(fb, 8, 8, 4);
    nova_grayscale(fb, 8, 8);
    uint32_t pal[4] = { 0xff000000u, 0xff0000ffu, 0xff00ff00u, 0xffff0000u };
    CHECK(nova_quantize(pal, 4, 0xfffe0000u) == 3);  /* near red */
    CHECK(nova_quantize(pal, 4, 0xff0000feu) == 1);  /* near blue */
}

static void test_save_migrate(void)
{
    uint8_t v1[] = { 'N','S',1,0, 1,42, 2,7 };
    uint8_t out[64];
    CHECK(nova_save_version(v1, sizeof(v1)) == 1);
    int n = nova_save_migrate(v1, sizeof(v1), out, sizeof(out));
    CHECK(n > 4);
    CHECK(out[0] == 'N' && out[2] == 3);
    uint8_t bad[] = { 'X','Y',1,0 };
    CHECK(nova_save_version(bad, sizeof(bad)) == -1);
}

static void test_tilemap_ex(void)
{
    nova_tilemap_ex t;
    CHECK(nova_tmx_init(&t, 16, 16, 2) == 0);
    t.cells[0][1 * 16 + 1] = 5;
    t.cells[0][1 * 16 + 2] = 5;
    nova_tmx_scroll(&t, 0, 8, 8);
    int mask = nova_tmx_autotile(&t, 0, 1, 1);
    CHECK(mask >= 0 && mask <= 15);
    nova_tmx_free(&t);
    CHECK(t.cells[0] == NULL);
}

static void test_collision2(void)
{
    nova_aabb a = { 0, 0, 10, 10 }, b = { 5, 5, 10, 10 }, c = { 100, 100, 5, 5 };
    CHECK(nova_aabb_overlap(a, b));
    CHECK(!nova_aabb_overlap(a, c));
    int tx, ty;
    int t = nova_aabb_sweep(a, 100, 100, c, &tx, &ty);
    CHECK(t >= 0);
    nova_spatial sp;
    CHECK(nova_spatial_init(&sp, 16, 64, 256) == 0);
    for (int i = 0; i < 50; i++) {
        nova_aabb box = { (i * 5) % 200, (i * 9) % 200, 8, 8 };
        nova_spatial_insert(&sp, box);
    }
    int out[64];
    int q = nova_spatial_query(&sp, a, out, 64);
    CHECK(q >= 0);
    nova_spatial_free(&sp);
}

static void test_coproc_batch_filter(void)
{
    uint8_t buf[64];
    int total = build_min_cart(buf);
    nova_machine mc;
    CHECK(nova_machine_load(&mc, buf, (size_t)total) == 0);

    /* coproc program: set r0=10,r1=20,r2=7; pixel r0,r1,r2; end
       encoding: CP_SET=1, CP_PIX=6, CP_END=0 */
    uint8_t prog2[] = { 1,0,10,0,0,0,  1,1,20,0,0,0,  1,2,7,0,0,0,  6,0,1,2,  0 };
    int ex = nova_coproc_run(&mc, prog2, sizeof(prog2));
    CHECK(ex == 4);

    nova_batch b;
    nova_batch_init(&b);
    nova_batch_add(&b, 0, 1, 1, 5);
    nova_batch_add(&b, 0, 2, 2, 1);
    nova_batch_add(&b, 0, 3, 3, 3);
    nova_batch_flush(&mc, &b);
    CHECK(b.n == 0);

    nova_filter_blur(mc.gfx.fb, NOVA_FB_W, NOVA_FB_H);
    nova_filter_sharpen(mc.gfx.fb, NOVA_FB_W, NOVA_FB_H);
    nova_filter_edges(mc.gfx.fb, NOVA_FB_W, NOVA_FB_H);

    nova_machine_free(&mc);
}

int main(void)
{
    test_hashmap();
    test_ring();
    test_bitset();
    test_tween();
    test_palette_ops();
    test_quadtree();
    test_fsm();
    test_config();
    test_soundfx();
    test_dither();
    test_save_migrate();
    test_tilemap_ex();
    test_collision2();
    test_coproc_batch_filter();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
