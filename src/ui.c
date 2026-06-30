#include "nova.h"
#include <string.h>

/* immediate-mode UI built on the rasterizer and the text renderer. */

void nova_ui_panel(nova_machine *mc, int x, int y, int w, int h, uint8_t fill, uint8_t border)
{
    nova_raster_fill(mc, x, y, w, h, fill);
    nova_raster_rect(mc, x, y, w, h, border);
}

int nova_ui_button(nova_machine *mc, int x, int y, int w, int h, const char *label, int hot)
{
    uint8_t fill = hot ? 9 : 6;
    nova_raster_fill(mc, x, y, w, h, fill);
    nova_raster_rect(mc, x, y, w, h, 15);
    if (label)
        nova_text_draw(mc, label, x + 3, y + (h - 8) / 2, 0);
    return hot;
}

void nova_ui_label(nova_machine *mc, int x, int y, const char *text, uint8_t color)
{
    if (text) nova_text_draw(mc, text, x, y, color);
}

void nova_ui_bar(nova_machine *mc, int x, int y, int w, int h, int value, int max, uint8_t color)
{
    nova_raster_rect(mc, x, y, w, h, 15);
    if (max <= 0) return;
    if (value < 0) value = 0;
    if (value > max) value = max;
    int fillw = (w - 2) * value / max;
    nova_raster_fill(mc, x + 1, y + 1, fillw, h - 2, color);
}

void nova_ui_progress(nova_machine *mc, int x, int y, int w, int value_q8)
{
    if (value_q8 < 0) value_q8 = 0;
    if (value_q8 > 256) value_q8 = 256;
    nova_raster_rect(mc, x, y, w, 6, 15);
    int fillw = (w - 2) * value_q8 / 256;
    nova_raster_fill(mc, x + 1, y + 1, fillw, 4, 11);
}
