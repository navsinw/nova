#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

/* build a minimal valid cart (header + CODE[halt]) so a machine can be loaded;
   this also allocates the framebuffer used by the raster tests. */
static int build_min_cart(uint8_t *buf, int with_sprite)
{
    memset(buf, 0, 256);
    memcpy(buf, "NOVA", 4);
    buf[4] = 2;
    int nchunks = with_sprite ? 2 : 1;
    buf[8] = (uint8_t)nchunks;
    uint32_t off = 18 + 12 * nchunks;
    /* CODE entry */
    buf[18] = 'C'; buf[19] = 'O'; buf[20] = 'D'; buf[21] = 'E';
    buf[22] = (uint8_t)off;
    buf[26] = 1;          /* size 1 */
    buf[off] = OP_HALT;
    int total = off + 1;
    if (with_sprite) {
        /* SPRT: nbanks=2,draw=2,anim=4, two 8x8 banks */
        int soff = total;
        buf[30] = 'S'; buf[31] = 'P'; buf[32] = 'R'; buf[33] = 'T';
        buf[34] = (uint8_t)soff;
        int sprt_hdr = 8 + 2 * (4 + 64);
        buf[38] = (uint8_t)(sprt_hdr & 0xff);
        buf[39] = (uint8_t)((sprt_hdr >> 8) & 0xff);
        uint8_t *s = buf + soff;
        s[0] = 2; s[2] = 2;          /* nbanks=2 draw=2 */
        s[4] = 4;                    /* anim_cnt=4 (le) */
        int p = 8;
        for (int bnk = 0; bnk < 2; bnk++) {
            s[p] = 8; s[p+2] = 8; p += 4;
            for (int i = 0; i < 64; i++) s[p++] = (uint8_t)(i + bnk);
        }
        total = soff + sprt_hdr;
    }
    return total;
}

static void test_matrix(void)
{
    nova_mat3 id; nova_mat3_identity(&id);
    nova_vec3 v = { NOVA_FP_ONE, 2 * NOVA_FP_ONE, 0 };
    nova_vec3 r = nova_mat3_apply(id, v);
    CHECK(r.x == v.x && r.y == v.y);
    nova_mat3 t = nova_mat3_translate(3 * NOVA_FP_ONE, 4 * NOVA_FP_ONE);
    nova_vec3 one = { 0, 0, NOVA_FP_ONE };
    nova_vec3 rt = nova_mat3_apply(t, one);
    CHECK(rt.x == 3 * NOVA_FP_ONE && rt.y == 4 * NOVA_FP_ONE);
    nova_mat3 s = nova_mat3_scale(2 * NOVA_FP_ONE, 2 * NOVA_FP_ONE);
    nova_mat3 ts = nova_mat3_mul(t, s);
    (void)ts;
    nova_vec3 a = { NOVA_FP_ONE, 0, 0 }, b = { NOVA_FP_ONE, 0, 0 };
    CHECK(nova_vec3_dot(a, b) == NOVA_FP_ONE);
    nova_vec3 sum = nova_vec3_add(a, b);
    CHECK(sum.x == 2 * NOVA_FP_ONE);
    nova_mat3 rz = nova_mat3_rotz(0);
    CHECK(rz.m[0] == NOVA_FP_ONE);
}

static void test_noise(void)
{
    nova_noise n1, n2;
    nova_noise_init(&n1, 99);
    nova_noise_init(&n2, 99);
    for (int i = 0; i < 50; i++) {
        int32_t a = nova_noise_value(&n1, i * 4096, i * 8192);
        int32_t b = nova_noise_value(&n2, i * 4096, i * 8192);
        CHECK(a == b);
    }
    int32_t f = nova_noise_fbm(&n1, 12345, 6789, 4);
    CHECK(f > -8 * NOVA_FP_ONE && f < 8 * NOVA_FP_ONE);
}

static void test_bignum(void)
{
    nova_u128 a, b;
    nova_u128_set(&a, 0xFFFFFFFFu);
    nova_u128_set(&b, 1);
    nova_u128_add(&a, &b);
    CHECK(a.w[0] == 0 && a.w[1] == 1);
    nova_u128_set(&a, 1000000);
    nova_u128_mul_u32(&a, 1000000);
    char dec[48];
    nova_u128_to_dec(&a, dec, sizeof(dec));
    CHECK(strcmp(dec, "1000000000000") == 0);
    nova_u128 z; nova_u128_set(&z, 0);
    nova_u128_to_dec(&z, dec, sizeof(dec));
    CHECK(strcmp(dec, "0") == 0);
    nova_u128 x, y; nova_u128_set(&x, 5); nova_u128_set(&y, 9);
    CHECK(nova_u128_cmp(&x, &y) < 0);
    CHECK(nova_u128_cmp(&y, &x) > 0);
    CHECK(nova_u128_cmp(&x, &x) == 0);
}

static void test_text(void)
{
    char out[256];
    CHECK(nova_text_measure("hello") == 5);
    CHECK(nova_text_measure("ab\ncde") == 3);
    int n = nova_text_wrap("the quick brown fox jumps", 10, out, sizeof(out));
    CHECK(n > 0);
    CHECK(strchr(out, '\n') != NULL);
    char line[16];
    strcpy(line, "hi");
    nova_text_align(line, 6, 2);
    CHECK(strlen(line) >= 2);
}

static void test_atlas(void)
{
    uint8_t img[64];
    for (int i = 0; i < 64; i++) img[i] = (uint8_t)(i < 32 ? 0 : 7);
    uint8_t packed[256], unp[64];
    int pn = nova_atlas_pack(img, 8, 8, packed, sizeof(packed));
    CHECK(pn > 4 && pn < 64);
    int un = nova_atlas_unpack(packed, pn, unp, sizeof(unp));
    CHECK(un == 64);
    CHECK(memcmp(img, unp, 64) == 0);
}

static void test_anim(void)
{
    nova_anim a;
    int frames[3] = { 10, 11, 12 };
    nova_anim_init(&a, frames, 3, 2, 1);
    CHECK(nova_anim_frame(&a) == 10);
    nova_anim_step(&a); /* timer 1 */
    CHECK(nova_anim_frame(&a) == 10);
    nova_anim_step(&a); /* advance */
    CHECK(nova_anim_frame(&a) == 11);
    for (int i = 0; i < 20; i++) nova_anim_step(&a);
    CHECK(a.nframes == 3);
    nova_anim_reset(&a);
    CHECK(a.cur == 0);
    nova_anim b;
    nova_anim_init(&b, frames, 3, 1, 0);
    for (int i = 0; i < 10; i++) nova_anim_step(&b);
    CHECK(b.done == 1 && nova_anim_frame(&b) == 12);
}

static void test_path(void)
{
    uint8_t grid[5 * 5];
    memset(grid, 0, sizeof(grid));
    grid[1*5+1] = 1; grid[1*5+2] = 1; grid[1*5+3] = 1; /* wall row */
    int out[64];
    int len = nova_path_find(grid, 5, 5, 0, 0, 4, 4, out, 64);
    CHECK(len > 0);
    CHECK(out[0] == 0 && out[1] == 0);
    CHECK(out[(len-1)*2] == 4 && out[(len-1)*2+1] == 4);
    /* blocked goal */
    grid[4*5+4] = 1;
    CHECK(nova_path_find(grid, 5, 5, 0, 0, 4, 4, out, 64) == 0);
}

static void test_wave(void)
{
    uint8_t blob[6 + 8];
    blob[0] = 4; blob[1] = blob[2] = blob[3] = 0;  /* nsamples=4 */
    blob[4] = 0x44; blob[5] = 0xAC;                /* rate ~ 44100 low bytes */
    for (int i = 0; i < 4; i++) { blob[6 + i*2] = (uint8_t)(i * 10); blob[6 + i*2 + 1] = 0; }
    nova_wave w;
    CHECK(nova_wave_load(&w, blob, sizeof(blob)) == 0);
    CHECK(w.nsamples == 4);
    int16_t out[4] = {0,0,0,0};
    int m = nova_wave_mix(&w, out, 4);
    CHECK(m == 4);
    int16_t out2[4] = {0,0,0,0};
    int m2 = nova_wave_mix(&w, out2, 4);
    CHECK(m2 == 0);
    CHECK(w.playing == 0);  /* consumed on the next mix past the end */
    nova_wave_free(&w);
    nova_wave bad;
    CHECK(nova_wave_load(&bad, blob, 3) != 0);
}

static void test_machine_render(void)
{
    uint8_t buf[256];
    int total = build_min_cart(buf, 1);
    nova_machine mc;
    CHECK(nova_machine_load(&mc, buf, (size_t)total) == 0);
    CHECK(mc.gfx.fb != NULL);

    nova_raster_clear(&mc, 3);
    CHECK(mc.gfx.fb[0] == 3);
    nova_raster_fill(&mc, 0, 0, 10, 10, 7);
    CHECK(mc.gfx.fb[0] == 7);
    nova_raster_rect(&mc, 2, 2, 5, 5, 9);
    nova_raster_line(&mc, 0, 0, 20, 20, 5);
    nova_raster_circle(&mc, 40, 40, 10, 6);
    nova_raster_pixel(&mc, 1000, 1000, 1);  /* off-screen, must not crash */

    /* gpu display list: clear, rect, sprite, end */
    uint8_t prog[] = {
        GOP_CLEAR, 0,
        GOP_RECT, 1,0, 1,0, 8,0, 8,0, 4,
        GOP_SPRITE, 0,0, 5,0, 5,0,
        GOP_END
    };
    int ex = nova_gpu_run(&mc, prog, sizeof(prog));
    CHECK(ex == 3);

    nova_blit_affine(&mc, 0, 80, 72, 45, NOVA_FP_ONE);

    nova_scene sc;
    nova_scene_init(&sc, NOVA_FB_W, NOVA_FB_H);
    int e = nova_scene_spawn(&sc, 10, 10, 0);
    CHECK(e == 0);
    for (int i = 0; i < 30; i++) { nova_scene_update(&sc); nova_scene_draw(&mc, &sc); }
    CHECK(sc.n == 1);
    nova_scene_free(&sc);

    nova_psys ps;
    nova_psys_init(&ps, 5);
    nova_psys_emit(&ps, 40, 40, 32, 8);
    for (int i = 0; i < 60; i++) { nova_psys_update(&ps); nova_psys_draw(&mc, &ps); }
    nova_psys_free(&ps);

    nova_mesh me;
    nova_mesh_cube(&me, NOVA_FP_ONE);
    me.angle_y = 32; me.angle_z = 10;
    nova_mesh_draw(&mc, &me, 80, 72, 7);
    CHECK(me.nverts == 8 && me.nedges == 12);

    CHECK(nova_console_exec(&mc, "rect 1 1 4 4 2") == 5);
    CHECK(nova_console_exec(&mc, "clear 0") == 1);
    CHECK(nova_console_exec(&mc, "bogus") == -1);

    nova_machine_free(&mc);
}

int main(void)
{
    test_matrix();
    test_noise();
    test_bignum();
    test_text();
    test_atlas();
    test_anim();
    test_path();
    test_wave();
    test_machine_render();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
