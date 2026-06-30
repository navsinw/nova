#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_grid(void)
{
    nova_grid g;
    CHECK(nova_grid_init(&g, 16, 16) == 0);
    nova_grid_fill(&g, 1);
    CHECK(nova_grid_get(&g, 5, 5) == 1);
    nova_grid_set(&g, 5, 5, 0);
    CHECK(nova_grid_get(&g, 5, 5) == 0);
    nova_grid_set(&g, 100, 100, 9);          /* OOB ignored */
    CHECK(nova_grid_get(&g, 100, 100) == 0);
    /* flood a 3x3 hole */
    nova_grid_set(&g, 6, 5, 0); nova_grid_set(&g, 5, 6, 0);
    int filled = nova_grid_flood(&g, 5, 5, 0, 7);
    CHECK(filled == 3);
    CHECK(nova_grid_get(&g, 5, 5) == 7);
    nova_grid_free(&g);
}

static void test_maze(void)
{
    nova_grid g;
    nova_grid_init(&g, 21, 21);
    CHECK(nova_maze_gen(&g, 1234) == 0);
    CHECK(nova_grid_get(&g, 0, 0) == 1);      /* border wall */
    CHECK(nova_grid_get(&g, 1, 1) == 0);      /* start carved */
    int floors = 0;
    for (int y = 0; y < 21; y++) for (int x = 0; x < 21; x++) if (!nova_grid_get(&g, x, y)) floors++;
    CHECK(floors > 20);
    nova_grid_free(&g);
}

static void test_dungeon_wfc(void)
{
    nova_grid g;
    nova_grid_init(&g, 48, 32);
    int rooms = nova_dungeon_gen(&g, 99, 16);
    CHECK(rooms > 0);
    nova_grid_free(&g);

    nova_grid w;
    nova_grid_init(&w, 24, 24);
    int collapsed = nova_wfc_gen(&w, 7, 5);
    CHECK(collapsed > 0);
    /* adjacency rule satisfied where collapsed */
    int violations = 0;
    for (int y = 0; y < 24; y++)
        for (int x = 1; x < 24; x++) {
            int a = nova_grid_get(&w, x, y), b = nova_grid_get(&w, x - 1, y);
            if (abs(a - b) > 1) violations++;
        }
    CHECK(violations == 0);
    nova_grid_free(&w);
}

static void test_base64(void)
{
    const uint8_t msg[] = "NOVA-8 fantasy console!";
    char enc[128];
    uint8_t dec[64];
    int n = nova_b64_encode(msg, (int)sizeof(msg) - 1, enc, sizeof(enc));
    CHECK(n > 0);
    int dn = nova_b64_decode(enc, n, dec, sizeof(dec));
    CHECK(dn == (int)sizeof(msg) - 1);
    CHECK(memcmp(dec, msg, dn) == 0);
}

static void test_lz77(void)
{
    uint8_t src[512];
    for (int i = 0; i < 512; i++) src[i] = (uint8_t)("ABCABCABC"[i % 9]);
    uint8_t comp[1024], dec[512];
    int cn = nova_lz_encode(src, 512, comp, sizeof(comp));
    CHECK(cn > 0 && cn < 512);              /* should compress */
    int dn = nova_lz_decode(comp, cn, dec, sizeof(dec));
    CHECK(dn == 512);
    CHECK(memcmp(src, dec, 512) == 0);
}

static void test_huffman(void)
{
    uint8_t src[400];
    for (int i = 0; i < 400; i++) src[i] = (uint8_t)((i % 5 == 0) ? 'x' : (i % 3 == 0 ? 'y' : 'z'));
    uint8_t comp[1024], dec[400];
    int cn = nova_huff_encode(src, 400, comp, sizeof(comp));
    CHECK(cn > 0);
    int dn = nova_huff_decode(comp, cn, dec, sizeof(dec));
    CHECK(dn == 400);
    CHECK(memcmp(src, dec, 400) == 0);

    /* single-symbol input */
    uint8_t one[10]; memset(one, 'q', 10);
    int c2 = nova_huff_encode(one, 10, comp, sizeof(comp));
    int d2 = nova_huff_decode(comp, c2, dec, sizeof(dec));
    CHECK(d2 == 10 && dec[0] == 'q' && dec[9] == 'q');
}

static void test_camera(void)
{
    nova_camera c;
    nova_camera_init(&c, 640, 480);
    nova_camera_follow(&c, 300, 200);
    for (int i = 0; i < 30; i++) nova_camera_update(&c);
    CHECK(c.x >= 0 && c.y >= 0);
    nova_camera_shake(&c, 4);
    CHECK(c.shake == 4);
    uint8_t buf[256];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, "NOVA", 4); buf[4]=2; buf[8]=1;
    int off = 18+12; buf[18]='C';buf[19]='O';buf[20]='D';buf[21]='E'; buf[22]=(uint8_t)off; buf[26]=1; buf[off]=OP_HALT;
    nova_machine mc;
    CHECK(nova_machine_load(&mc, buf, off+1) == 0);
    nova_camera_apply(&mc, &c);
    nova_machine_free(&mc);
}

static void test_timer_evq_score(void)
{
    nova_timer t;
    nova_timer_set(&t, 3);
    int fires = 0;
    for (int i = 0; i < 10; i++) fires += nova_timer_update(&t);
    CHECK(fires == 3);
    nova_timer_stop(&t);
    CHECK(nova_timer_update(&t) == 0);

    nova_eventq q;
    CHECK(nova_evq_init(&q, 8) == 0);
    nova_evq_push(&q, 1, 10, 20);
    nova_evq_push(&q, 2, 30, 40);
    nova_event e;
    CHECK(nova_evq_pop(&q, &e) == 0 && e.type == 1 && e.a == 10);
    CHECK(nova_evq_pop(&q, &e) == 0 && e.type == 2 && e.b == 40);
    CHECK(nova_evq_pop(&q, &e) != 0);
    nova_evq_free(&q);

    nova_score s;
    nova_score_init(&s);
    nova_score_add(&s, 100);
    nova_score_combo(&s, 3);
    nova_score_add(&s, 100);             /* +300 */
    char buf[48];
    nova_score_str(&s, buf, sizeof(buf));
    CHECK(strcmp(buf, "400") == 0);
    CHECK(nova_score_commit_high(&s) == 1);
    CHECK(nova_score_commit_high(&s) == 0);
}

int main(void)
{
    test_grid();
    test_maze();
    test_dungeon_wfc();
    test_base64();
    test_lz77();
    test_huffman();
    test_camera();
    test_timer_evq_score();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
