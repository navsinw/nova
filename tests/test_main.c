#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail;
static int g_run;

#define CHECK(cond) do { g_run++; if (!(cond)) { g_fail++; \
    printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void test_crc(void)
{
    const uint8_t a[] = "123456789";
    CHECK(nova_crc32(a, 9) == 0xCBF43926u);
    CHECK(nova_crc16(a, 9) == 0x29B1);
    CHECK(nova_adler32((const uint8_t*)"", 0) == 1u);
    CHECK(nova_fnv1a(a, 9) != 0);
    /* determinism */
    CHECK(nova_crc32(a, 9) == nova_crc32(a, 9));
}

static void test_rng(void)
{
    nova_rng r1, r2;
    nova_rng_seed(&r1, 42);
    nova_rng_seed(&r2, 42);
    for (int i = 0; i < 100; i++)
        CHECK(nova_rng_next(&r1) == nova_rng_next(&r2));
    nova_rng_seed(&r1, 7);
    for (int i = 0; i < 1000; i++) {
        int v = nova_rng_range(&r1, 10, 20);
        CHECK(v >= 10 && v < 20);
    }
    int arr[8]; for (int i = 0; i < 8; i++) arr[i] = i;
    nova_rng_shuffle(&r1, arr, 8);
    int sum = 0; for (int i = 0; i < 8; i++) sum += arr[i];
    CHECK(sum == 28);
}

static void test_math(void)
{
    CHECK(nova_fp_mul(NOVA_FP_ONE, NOVA_FP_ONE) == NOVA_FP_ONE);
    CHECK(nova_fp_mul(2 * NOVA_FP_ONE, 3 * NOVA_FP_ONE) == 6 * NOVA_FP_ONE);
    CHECK(nova_fp_div(6 * NOVA_FP_ONE, 2 * NOVA_FP_ONE) == 3 * NOVA_FP_ONE);
    CHECK(nova_fp_div(1, 0) != 0);
    int32_t s = nova_fp_sqrt(4 * NOVA_FP_ONE);
    CHECK(s > NOVA_FP_ONE && s < 3 * NOVA_FP_ONE);
    CHECK(nova_lerp(0, 100, 5, 10) == 50);
    CHECK(nova_lerp(0, 100, 20, 10) == 100);
    CHECK(nova_clampi(15, 0, 10) == 10);
    CHECK(nova_clampi(-5, 0, 10) == 0);
    int32_t sv = nova_fp_sin(0);
    CHECK(sv > -NOVA_FP_ONE - 1 && sv < NOVA_FP_ONE + 1);
}

static void test_strtab(void)
{
    nova_strtab st;
    nova_strtab_init(&st);
    int a = nova_strtab_add(&st, "alpha");
    int b = nova_strtab_add(&st, "beta");
    int a2 = nova_strtab_add(&st, "alpha");
    CHECK(a == 0 && b == 1 && a2 == 0);
    CHECK(st.n == 2);
    CHECK(strcmp(nova_strtab_get(&st, b), "beta") == 0);
    CHECK(nova_strtab_find(&st, "gamma") < 0);
    for (int i = 0; i < 100; i++) {
        char buf[16]; snprintf(buf, sizeof(buf), "s%d", i);
        nova_strtab_add(&st, buf);
    }
    CHECK(st.n == 102);
    nova_strtab_free(&st);
    CHECK(st.items == NULL);
}

static void test_inflate(void)
{
    uint8_t out[256];
    /* RLE: literal run of 3 bytes (ctrl=2 -> lit count 3) then a repeat */
    uint8_t rle[] = { 0x02, 'a', 'b', 'c', 0x83, 'z' };  /* abc + zzzz */
    int n = nova_rle_decode(rle, sizeof(rle), out, sizeof(out));
    CHECK(n == 7);
    CHECK(out[0] == 'a' && out[2] == 'c' && out[3] == 'z' && out[6] == 'z');

    /* LZ: all literal flags */
    uint8_t lz[] = { 0xff, 'h', 'e', 'l', 'l', 'o', '!', '?', 0x00 };
    n = nova_lz_decode(lz, sizeof(lz), out, sizeof(out));
    CHECK(n >= 8);
    CHECK(out[0] == 'h' && out[4] == 'o');

    /* outcap clamp */
    uint8_t big[2] = { 0xff, 'x' };
    n = nova_lz_decode(big, sizeof(big), out, 0);
    CHECK(n == 0);
}

static void test_disasm(void)
{
    uint8_t code[] = { OP_PUSH, 0x01, 0, 0, 0, OP_PUSHB, 0x05, OP_ADD, OP_HALT };
    char line[96];
    int adv = nova_disasm_one(code, sizeof(code), 0, line, sizeof(line));
    CHECK(adv == 5);
    CHECK(strstr(line, "push") != NULL);
    adv = nova_disasm_one(code, sizeof(code), 5, line, sizeof(line));
    CHECK(adv == 2);
    CHECK(strstr(line, "pushb") != NULL);
}

static void test_cart(void)
{
    /* hand-build a minimal valid cart: header + 1 dir entry + CODE(halt) */
    uint8_t buf[64];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, "NOVA", 4);
    buf[4] = 2; buf[5] = 0;        /* version */
    buf[8] = 1; buf[9] = 0;        /* chunk_cnt = 1 */
    /* entry_pc=0 at 10..13, crc at 14..17 */
    uint32_t off = 18 + 12;
    /* dir entry at 18: tag CODE, offset, size */
    buf[18] = 'C'; buf[19] = 'O'; buf[20] = 'D'; buf[21] = 'E';
    buf[22] = (uint8_t)off; buf[26] = 1;   /* offset=off, size=1 */
    buf[off] = OP_HALT;
    size_t total = off + 1;

    nova_cart cart;
    CHECK(nova_cart_open(&cart, buf, total) == 0);
    const uint8_t *c; uint32_t cl;
    CHECK(nova_cart_find(&cart, TAG_CODE, &c, &cl) == 0);
    CHECK(cl == 1 && c[0] == OP_HALT);
    CHECK(nova_cart_find(&cart, TAG_SND, &c, &cl) != 0);
    nova_cart_close(&cart);

    /* too short / bad magic rejected */
    CHECK(nova_cart_open(&cart, buf, 4) != 0);
    uint8_t bad[32]; memset(bad, 0, sizeof(bad));
    CHECK(nova_cart_open(&cart, bad, sizeof(bad)) != 0);
}

int main(void)
{
    test_crc();
    test_rng();
    test_math();
    test_strtab();
    test_inflate();
    test_disasm();
    test_cart();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
