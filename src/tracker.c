#include "nova.h"
#include <stdlib.h>
#include <string.h>

static uint16_t t16(const uint8_t *p){ return (uint16_t)(p[0]|(p[1]<<8)); }

int nova_tracker_init(nova_machine *mc)
{
    memset(&mc->trk, 0, sizeof(mc->trk));
    const uint8_t *p; uint32_t n;
    if (nova_cart_find(&mc->cart, TAG_PATN, &p, &n) != 0) return 0;
    if (n < 4) return 0;

    int np = t16(p);
    int ol = t16(p + 2);
    if (np <= 0 || np > 256 || ol <= 0 || ol > 1024) return 0;

    uint32_t off = 4;
    if (off + (uint32_t)ol > n) return 0;
    mc->trk.order = (uint8_t*)malloc(ol);
    memcpy(mc->trk.order, p + off, ol);
    mc->trk.order_len = ol;
    off += ol;

    mc->trk.patterns = (nova_pattern*)calloc(np, sizeof(nova_pattern));
    mc->trk.npatterns = np;
    for (int i = 0; i < np; i++) {
        if (off + 2 > n) { mc->trk.npatterns = i; break; }
        int rows = t16(p + off);
        off += 2;
        if (rows < 0 || rows > 4096) { mc->trk.npatterns = i; break; }
        uint32_t bytes = (uint32_t)rows * 4u;
        if (off + bytes > n) { mc->trk.npatterns = i; break; }
        mc->trk.patterns[i].rows = (uint8_t*)malloc(bytes ? bytes : 1);
        memcpy(mc->trk.patterns[i].rows, p + off, bytes);
        mc->trk.patterns[i].num_rows = rows;
        mc->trk.patterns[i].row_stride = 4;
        off += bytes;
    }
    mc->trk.speed = 6;
    return 0;
}

void nova_tracker_free(nova_machine *mc)
{
    if (mc->trk.next_buf) { free(mc->trk.next_buf); mc->trk.next_buf = NULL; }
    for (int i = 0; i < mc->trk.npatterns; i++)
        free(mc->trk.patterns[i].rows);
    free(mc->trk.patterns);
    free(mc->trk.order);
    mc->trk.patterns = NULL;
    mc->trk.order = NULL;
}

void nova_tracker_play(nova_machine *mc, int order)
{
    if (mc->trk.npatterns <= 0 || mc->trk.order_len <= 0) return;
    if (order < 0 || order >= mc->trk.order_len) order = 0;
    mc->trk.cur_order = order;
    mc->trk.cur_row = 0;
    mc->trk.tick_ctr = 0;
    mc->trk.playing = 1;
    int pi = mc->trk.order[order];
    if (pi >= 0 && pi < mc->trk.npatterns) {
        mc->trk.cur_pat = pi;
        mc->trk.cur_buf = &mc->trk.patterns[pi];
    }
}

void nova_tracker_tick(nova_machine *mc)
{
    nova_tracker *t = &mc->trk;
    if (!t->playing || !t->cur_buf) return;
    t->tick_ctr++;
    if (t->tick_ctr < t->speed) return;
    t->tick_ctr = 0;

    nova_pattern *p = t->cur_buf;
    const uint8_t *row = p->rows + t->cur_row * 4;
    uint8_t note = row[0], instr = row[1], cmd = row[2], param = row[3];

    if (note > 0) {
        nova_synth_voice(mc, 0, instr);
        nova_synth_note(mc, 0, note);
    }
    if (cmd == 0x0B) {
        t->next_buf = (nova_pattern*)malloc(sizeof(nova_pattern));
        *t->next_buf = *p;
    } else if (cmd == 0x0D) {
        t->cur_row = param;
        return;
    } else if (cmd == 0x0E) {
        int ord = param;
        if (ord >= 0 && ord < t->order_len) {
            t->cur_order = ord;
            int pi = t->order[ord];
            if (pi >= 0 && pi < t->npatterns) { t->cur_pat = pi; t->cur_buf = &t->patterns[pi]; }
        }
        return;
    }

    t->cur_row++;
    if (t->cur_row >= p->num_rows) {
        t->cur_row = 0;
        t->cur_order++;
        if (t->cur_order >= t->order_len) {
            t->playing = 0;
            if (t->next_buf) {
                free(t->next_buf->rows);
                free(t->next_buf);
                t->next_buf = NULL;
            }
            return;
        }
        int pi = t->order[t->cur_order];
        if (pi >= 0 && pi < t->npatterns) {
            t->cur_pat = pi;
            t->cur_buf = &t->patterns[pi];
        }
    }
}
