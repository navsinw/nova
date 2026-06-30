#include "nova.h"
#include <stdio.h>
#include <string.h>

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_input(void)
{
    nova_input in;
    nova_input_init(&in);
    nova_input_set(&in, 0x1);
    CHECK(nova_input_pressed(&in, 0));
    CHECK(nova_input_held(&in, 0));
    CHECK(!nova_input_released(&in, 0));
    nova_input_update(&in);
    nova_input_set(&in, 0x1);
    CHECK(!nova_input_pressed(&in, 0));     /* held, not newly pressed */
    CHECK(nova_input_held(&in, 0));
    nova_input_update(&in);
    nova_input_set(&in, 0x0);
    CHECK(nova_input_released(&in, 0));
}

static void test_dialogue(void)
{
    nova_dialogue d;
    nova_dialogue_set(&d, "hi there", 1);
    char buf[32];
    for (int i = 0; i < 3; i++) nova_dialogue_advance(&d);
    int vis = nova_dialogue_visible(&d, buf, sizeof(buf));
    CHECK(vis == 3);
    CHECK(strncmp(buf, "hi ", 3) == 0);
    nova_dialogue_skip(&d);
    CHECK(d.done == 1);
    nova_dialogue_visible(&d, buf, sizeof(buf));
    CHECK(strcmp(buf, "hi there") == 0);
}

static void test_inventory(void)
{
    nova_inventory inv;
    nova_inv_init(&inv, 10);
    CHECK(nova_inv_add(&inv, 1, 25) == 25);   /* spills into 3 stacks of <=10 */
    CHECK(nova_inv_count(&inv, 1) == 25);
    CHECK(nova_inv_add(&inv, 2, 5) == 5);
    CHECK(nova_inv_remove(&inv, 1, 12) == 12);
    CHECK(nova_inv_count(&inv, 1) == 13);
    CHECK(nova_inv_count(&inv, 2) == 5);
    CHECK(nova_inv_remove(&inv, 99, 1) == 0);
}

static void test_tilecollide(void)
{
    uint8_t grid[8 * 8];
    memset(grid, 0, sizeof(grid));
    grid[3 * 8 + 4] = 1;   /* solid tile at cell (4,3) -> pixels x32..39 y24..31 */
    int x = 16, y = 24;
    int hit = nova_tile_resolve(grid, 8, 8, 1, &x, &y, 8, 8, 20, 0);
    CHECK(hit & 1);        /* blocked horizontally before reaching the wall */
    CHECK(x + 8 <= 32 + 1);
    int x2 = 0, y2 = 0;
    int h2 = nova_tile_resolve(grid, 8, 8, 1, &x2, &y2, 8, 8, 4, 4);
    CHECK(x2 == 4 && y2 == 4 && h2 == 0);    /* free move */
}

int main(void)
{
    test_input();
    test_dialogue();
    test_inventory();
    test_tilecollide();
    printf("%d checks, %d failures\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
