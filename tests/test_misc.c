#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_vec2(void)
{
    nova_vec2 a = { NOVA_FP_ONE, 0 }, b = { 0, NOVA_FP_ONE };
    nova_vec2 s = nova_v2_add(a, b);
    CHECK(s.x == NOVA_FP_ONE && s.y == NOVA_FP_ONE);
    CHECK(nova_v2_dot(a, b) == 0);
    CHECK(nova_v2_cross(a, b) == NOVA_FP_ONE);
    nova_vec2 d = nova_v2_sub(s, a);
    CHECK(d.x == 0 && d.y == NOVA_FP_ONE);
    int32_t len = nova_v2_len(a);
    CHECK(len > NOVA_FP_ONE - 256 && len < NOVA_FP_ONE + 256);
    nova_vec2 n = nova_v2_normalize((nova_vec2){ 3 * NOVA_FP_ONE, 4 * NOVA_FP_ONE });
    int32_t nl = nova_v2_len(n);
    CHECK(nl > NOVA_FP_ONE - 1024 && nl < NOVA_FP_ONE + 1024);
    nova_vec2 r = nova_v2_rotate(a, 64);   /* ~90 deg */
    CHECK(r.x > -2048 && r.x < 2048);
}

static void test_rect(void)
{
    nova_rect a = { 0, 0, 10, 10 }, b = { 5, 5, 10, 10 }, out;
    CHECK(nova_rect_contains(a, 5, 5));
    CHECK(!nova_rect_contains(a, 10, 10));
    CHECK(nova_rect_intersect(a, b, &out));
    CHECK(out.x == 5 && out.y == 5 && out.w == 5 && out.h == 5);
    nova_rect c = { 100, 100, 5, 5 };
    CHECK(!nova_rect_intersect(a, c, &out));
    nova_rect u = nova_rect_union(a, b);
    CHECK(u.x == 0 && u.w == 15);
    nova_rect bounds = { 0, 0, 8, 8 };
    nova_rect cl = nova_rect_clamp(a, bounds);
    CHECK(cl.w == 8 && cl.h == 8);
}

static void test_color(void)
{
    uint32_t red = nova_hsv_to_rgb(0, 255, 255);
    CHECK(((red >> 16) & 0xff) > 200);
    CHECK((red & 0xff) < 60);
    int h, s, v;
    nova_rgb_to_hsv(0xff00ff00u, &h, &s, &v);
    CHECK(h > 100 && h < 140);          /* green ~120 */
    CHECK(v == 255);
    uint32_t mid = nova_color_lerp_hsv(0xff000000u, 0xffffffffu, 128);
    CHECK((mid & 0xff000000u) == 0xff000000u);
}

static void test_sort(void)
{
    int a[10] = { 5, 3, 8, 1, 9, 2, 7, 0, 6, 4 };
    nova_sort_int(a, 10);
    for (int i = 0; i < 10; i++) CHECK(a[i] == i);
    CHECK(nova_bsearch_int(a, 10, 7) == 7);
    CHECK(nova_bsearch_int(a, 10, 99) == -1);
    uint32_t u[5] = { 50, 10, 40, 20, 30 };
    nova_sort_u32(u, 5);
    CHECK(u[0] == 10 && u[4] == 50);
    /* larger random-ish array */
    int big[200];
    for (int i = 0; i < 200; i++) big[i] = (i * 7919) % 1000;
    nova_sort_int(big, 200);
    int ok = 1;
    for (int i = 1; i < 200; i++) if (big[i] < big[i-1]) ok = 0;
    CHECK(ok);
}

static void test_list(void)
{
    nova_list l;
    nova_list_init(&l);
    nova_list_push_back(&l, 1);
    nova_list_push_back(&l, 2);
    nova_list_push_front(&l, 0);
    CHECK(l.count == 3);
    int v;
    CHECK(nova_list_pop_front(&l, &v) == 0 && v == 0);
    nova_list_push_back(&l, 2);
    CHECK(nova_list_remove(&l, 2) == 2);
    CHECK(l.count == 1);
    nova_list_free(&l);
    CHECK(l.count == 0);
}

static void test_hashes_hexdump(void)
{
    const uint8_t *m = (const uint8_t*)"hello world";
    CHECK(nova_djb2(m, 11) != 0);
    CHECK(nova_sdbm(m, 11) != 0);
    CHECK(nova_murmur3(m, 11, 0) == nova_murmur3(m, 11, 0));
    CHECK(nova_murmur3(m, 11, 1) != nova_murmur3(m, 11, 0));
    char out[512];
    int n = nova_hexdump(m, 11, out, sizeof(out));
    CHECK(n > 0);
    CHECK(strstr(out, "hello world") != NULL || strstr(out, "hello") != NULL);
}

static void test_json(void)
{
    const char *txt = "{ \"w\": 160, \"h\": 144, \"name\": \"nova\", \"on\": true, \"tags\": [1,2,3] }";
    nova_json j;
    int root = nova_json_parse(&j, txt, (int)strlen(txt));
    CHECK(root >= 0);
    int w = nova_json_get(&j, root, "w");
    CHECK(w >= 0 && j.nodes[w].type == NJSON_NUM && j.nodes[w].num == 160);
    int name = nova_json_get(&j, root, "name");
    CHECK(name >= 0 && j.nodes[name].type == NJSON_STR && strcmp(j.nodes[name].str, "nova") == 0);
    int on = nova_json_get(&j, root, "on");
    CHECK(on >= 0 && j.nodes[on].type == NJSON_BOOL && j.nodes[on].num == 1);
    int tags = nova_json_get(&j, root, "tags");
    CHECK(tags >= 0 && j.nodes[tags].type == NJSON_ARR);
    int cnt = 0;
    for (int c = j.nodes[tags].first_child; c >= 0; c = j.nodes[c].next_sibling) cnt++;
    CHECK(cnt == 3);
    nova_json_free(&j);

    /* malformed input must not crash and should report error */
    nova_json bad;
    CHECK(nova_json_parse(&bad, "{ \"x\": ", 7) < 0);
    nova_json_free(&bad);
}

int main(void)
{
    test_vec2();
    test_rect();
    test_color();
    test_sort();
    test_list();
    test_hashes_hexdump();
    test_json();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
