#include "nova.h"
#include <stdio.h>
#include <stdlib.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_heap(void)
{
    nova_heap h;
    CHECK(nova_heap_init(&h, 4) == 0);
    int keys[8] = { 5, 1, 8, 3, 9, 2, 7, 4 };
    for (int i = 0; i < 8; i++) nova_heap_push(&h, keys[i], i);
    int prev = -1, k, v, cnt = 0;
    while (nova_heap_pop(&h, &k, &v) == 0) {
        CHECK(k >= prev);
        prev = k;
        cnt++;
    }
    CHECK(cnt == 8);
    nova_heap_free(&h);
}

static void test_deque(void)
{
    nova_deque d;
    CHECK(nova_deque_init(&d, 4) == 0);
    nova_deque_push_back(&d, 1);
    nova_deque_push_back(&d, 2);
    nova_deque_push_front(&d, 0);
    CHECK(d.count == 3);
    int v;
    CHECK(nova_deque_pop_front(&d, &v) == 0 && v == 0);
    CHECK(nova_deque_pop_back(&d, &v) == 0 && v == 2);
    CHECK(nova_deque_pop_front(&d, &v) == 0 && v == 1);
    CHECK(nova_deque_pop_back(&d, &v) != 0);
    nova_deque_free(&d);
}

static void test_automata(void)
{
    nova_grid g;
    nova_grid_init(&g, 32, 32);
    int floor = nova_ca_cave(&g, 1234, 4, 45);
    CHECK(floor > 0 && floor < 32 * 32);
    /* cave generation is deterministic for a given seed */
    nova_grid g2;
    nova_grid_init(&g2, 32, 32);
    int floor2 = nova_ca_cave(&g2, 1234, 4, 45);
    CHECK(floor == floor2);
    int changed = nova_ca_step(&g, 1 << 3, (1 << 2) | (1 << 3));
    CHECK(changed >= 0);
    nova_grid_free(&g2);
    nova_grid_free(&g);
}

static void test_dice(void)
{
    nova_rng r;
    nova_rng_seed(&r, 5);
    for (int i = 0; i < 100; i++) {
        int roll = nova_roll(&r, 6, 2);
        CHECK(roll >= 2 && roll <= 12);
    }
    int weights[4] = { 0, 10, 0, 0 };
    CHECK(nova_weighted_choice(&r, weights, 4) == 1);  /* only index 1 has weight */

    nova_bag bag;
    CHECK(nova_bag_init(&bag, 4, 9) == 0);
    nova_bag_add(&bag, 10);
    nova_bag_add(&bag, 20);
    nova_bag_add(&bag, 30);
    int seen[3] = { 0, 0, 0 };
    for (int i = 0; i < 3; i++) {
        int d = nova_bag_draw(&bag);
        if (d == 10) seen[0]++; else if (d == 20) seen[1]++; else if (d == 30) seen[2]++;
    }
    CHECK(seen[0] == 1 && seen[1] == 1 && seen[2] == 1);  /* each drawn once per cycle */
    nova_bag_free(&bag);
}

static void test_steering(void)
{
    nova_vec2 pos = { 0, 0 }, tgt = { 10 * NOVA_FP_ONE, 0 };
    nova_vec2 seek = nova_steer_seek(pos, tgt, 2 * NOVA_FP_ONE);
    CHECK(seek.x > 0);                       /* heads toward target (+x) */
    nova_vec2 flee = nova_steer_flee(pos, tgt, 2 * NOVA_FP_ONE);
    CHECK(flee.x < 0);                       /* heads away */
    nova_vec2 near = { 9 * NOVA_FP_ONE, 0 };
    nova_vec2 arr = nova_steer_arrive(near, tgt, 4 * NOVA_FP_ONE, 5 * NOVA_FP_ONE);
    CHECK(arr.x > 0 && arr.x <= 4 * NOVA_FP_ONE);  /* slows within radius */
    nova_rng r; nova_rng_seed(&r, 3);
    nova_vec2 v = { NOVA_FP_ONE, 0 };
    nova_vec2 w = nova_steer_wander(v, &r, NOVA_FP_ONE);
    CHECK(w.x != 0 || w.y != 0);
}

int main(void)
{
    test_heap();
    test_deque();
    test_automata();
    test_dice();
    test_steering();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
