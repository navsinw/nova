#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static int build_min_cart(uint8_t *buf)
{
    memset(buf, 0, 64);
    memcpy(buf, "NOVA", 4); buf[4]=2; buf[8]=1;
    uint32_t off = 18+12;
    buf[18]='C';buf[19]='O';buf[20]='D';buf[21]='E'; buf[22]=(uint8_t)off; buf[26]=1; buf[off]=OP_HALT;
    return (int)off+1;
}

static void test_bezier_poly(void)
{
    uint8_t buf[64];
    int total = build_min_cart(buf);
    nova_machine mc;
    CHECK(nova_machine_load(&mc, buf, (size_t)total) == 0);

    nova_vec2 a = {10<<16,10<<16}, b = {40<<16,0}, c = {80<<16,60<<16}, d = {120<<16,20<<16};
    nova_vec2 mid = nova_bezier_cubic(a, b, c, d, NOVA_FP_ONE/2);
    CHECK(mid.x > 0 && mid.y > 0);
    nova_bezier_draw(&mc, a, b, c, d, 32, 7);

    int tri[6] = { 20, 20, 80, 30, 40, 90 };
    CHECK(nova_poly_contains(tri, 3, 45, 45));
    CHECK(!nova_poly_contains(tri, 3, 0, 0));
    CHECK(nova_poly_area2(tri, 3) > 0);
    nova_poly_fill(&mc, tri, 3, 5);

    nova_machine_free(&mc);
}

static void test_astar_cost(void)
{
    uint8_t cost[8*8];
    for (int i = 0; i < 64; i++) cost[i] = 1;
    /* a costly diagonal wall of high cost, plus one blocked cell */
    cost[3*8+3] = 9; cost[3*8+4] = 9;
    cost[5*8+5] = 0;  /* blocked */
    int out[128];
    int len = nova_astar_cost(cost, 8, 8, 0, 0, 7, 7, out, 128);
    CHECK(len > 0);
    CHECK(out[0] == 0 && out[1] == 0);
    CHECK(out[(len-1)*2] == 7 && out[(len-1)*2+1] == 7);
    /* path should avoid the blocked cell */
    int hits = 0;
    for (int i = 0; i < len; i++) if (out[i*2] == 5 && out[i*2+1] == 5) hits++;
    CHECK(hits == 0);
}

static void test_state_stack(void)
{
    nova_sstack s;
    nova_sstack_init(&s);
    CHECK(nova_sstack_top(&s) == -1);
    nova_sstack_push(&s, 1);
    nova_sstack_push(&s, 2);
    CHECK(nova_sstack_top(&s) == 2);
    CHECK(nova_sstack_pop(&s) == 2);
    CHECK(nova_sstack_top(&s) == 1);
    CHECK(nova_sstack_pop(&s) == 1);
    CHECK(nova_sstack_pop(&s) == -1);
}

static void test_track(void)
{
    nova_track t;
    nova_track_init(&t);
    nova_track_add(&t, 0, 0);
    nova_track_add(&t, 10, 100);
    nova_track_add(&t, 20, 0);
    CHECK(nova_track_sample(&t, 5) == 50);
    CHECK(nova_track_sample(&t, 0) == 0);
    CHECK(nova_track_sample(&t, 15) == 50);
    CHECK(nova_track_sample(&t, 100) == 0);
    /* out-of-order insert keeps ordering */
    nova_track_add(&t, 5, 25);
    CHECK(nova_track_sample(&t, 5) == 25);
}

static void test_spring(void)
{
    nova_spring s;
    nova_spring_init(&s, 0, NOVA_FP_ONE/8, NOVA_FP_ONE/4);
    s.target = 100 * NOVA_FP_ONE;
    for (int i = 0; i < 300; i++) nova_spring_update(&s);
    int32_t settle = s.pos >> 16;
    CHECK(settle > 90 && settle < 110);   /* converges near target */
}

int main(void)
{
    test_bezier_poly();
    test_astar_cost();
    test_state_stack();
    test_track();
    test_spring();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
