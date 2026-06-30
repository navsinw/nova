#include "nova.h"
#include <stdlib.h>
#include <string.h>

int nova_font_init(nova_machine *mc)
{
    memset(&mc->font, 0, sizeof(mc->font));
    const uint8_t *p; uint32_t n;
    if (nova_cart_find(&mc->cart, TAG_FONT, &p, &n) != 0) return 0;
    if (n < 2) return 0;

    int ng = p[0] | (p[1] << 8);
    if (ng <= 0 || ng > 1024) return 0;
    mc->font.glyphs = (nova_glyph*)calloc(ng, sizeof(nova_glyph));
    mc->font.nglyphs = ng;

    uint32_t off = 2;
    for (int i = 0; i < ng; i++) {
        if (off + 4 > n) { mc->font.nglyphs = i; break; }
        int w = p[off], h = p[off+1];
        int comp = p[off+2];
        int nc = p[off+3];
        off += 4;
        nova_glyph *g = &mc->font.glyphs[i];
        g->w = w; g->h = h; g->is_composite = comp;
        if (comp) {
            if (off + (uint32_t)nc > n) { mc->font.nglyphs = i; break; }
            g->comps = (uint8_t*)malloc(nc ? nc : 1);
            memcpy(g->comps, p + off, nc);
            g->ncomps = nc;
            off += nc;
        } else {
            uint32_t px = (uint32_t)w * (uint32_t)h;
            if (off + px > n) { mc->font.nglyphs = i; break; }
            g->bitmap = (uint8_t*)malloc(px ? px : 1);
            memcpy(g->bitmap, p + off, px);
            off += px;
        }
    }

    mc->font.cache_cap = 4;
    mc->font.run_cap = 64;
    mc->font.run = (uint8_t*)malloc(mc->font.run_cap);
    return 0;
}

void nova_font_free(nova_machine *mc)
{
    for (int i = 0; i < mc->font.nglyphs; i++) {
        free(mc->font.glyphs[i].bitmap);
        free(mc->font.glyphs[i].comps);
    }
    free(mc->font.glyphs);
    nova_glyph_cache_e *e = mc->font.lru_head;
    while (e) { nova_glyph_cache_e *nx = e->lru_next; free(e->raster); free(e); e = nx; }
    free(mc->font.run);
    mc->font.glyphs = NULL;
    mc->font.run = NULL;
}

static nova_glyph_cache_e *cache_find(nova_font *f, int glyph)
{
    for (nova_glyph_cache_e *e = f->lru_head; e; e = e->lru_next)
        if (e->glyph == glyph) return e;
    return NULL;
}

static void lru_unlink(nova_font *f, nova_glyph_cache_e *e)
{
    if (e->lru_prev) e->lru_prev->lru_next = e->lru_next; else f->lru_head = e->lru_next;
    if (e->lru_next) e->lru_next->lru_prev = e->lru_prev; else f->lru_tail = e->lru_prev;
    e->lru_prev = e->lru_next = NULL;
}

static void lru_push_tail(nova_font *f, nova_glyph_cache_e *e)
{
    e->lru_prev = f->lru_tail;
    e->lru_next = NULL;
    if (f->lru_tail) f->lru_tail->lru_next = e; else f->lru_head = e;
    f->lru_tail = e;
}

static void cache_insert(nova_font *f, int glyph, uint8_t *raster, int size)
{
    while (f->cache_used >= f->cache_cap && f->lru_head) {
        nova_glyph_cache_e *victim = f->lru_head;
        lru_unlink(f, victim);
        free(victim->raster);
        free(victim);
        f->cache_used--;
    }
    nova_glyph_cache_e *e = (nova_glyph_cache_e*)calloc(1, sizeof(*e));
    e->glyph = glyph;
    e->raster = raster;
    e->size = size;
    lru_push_tail(f, e);
    f->cache_used++;
}

static uint8_t *font_raster(nova_machine *mc, int glyph, int depth)
{
    nova_font *f = &mc->font;
    if (depth > 16) return NULL;
    if (glyph < 0 || glyph >= f->nglyphs) return NULL;

    nova_glyph_cache_e *hit = cache_find(f, glyph);
    if (hit) { lru_unlink(f, hit); lru_push_tail(f, hit); return hit->raster; }

    nova_glyph *g = &f->glyphs[glyph];
    int size = g->w * g->h;
    if (size <= 0) size = 1;
    uint8_t *raster = (uint8_t*)calloc(size, 1);

    if (g->is_composite) {
        int npc = g->ncomps; if (npc > 32) npc = 32;
        uint8_t *parts[32]; int psz[32];
        for (int k = 0; k < npc; k++) {
            int cid = g->comps[k];
            if (cid >= 0 && cid < f->nglyphs) {
                parts[k] = font_raster(mc, cid, depth + 1);
                psz[k] = f->glyphs[cid].w * f->glyphs[cid].h;
            } else { parts[k] = NULL; psz[k] = 0; }
        }
        for (int k = 0; k < npc; k++) {
            if (!parts[k]) continue;
            int lim = psz[k] < size ? psz[k] : size;
            for (int i = 0; i < lim; i++)
                raster[i] = (uint8_t)(raster[i] + parts[k][i]);
        }
    } else if (g->bitmap) {
        int lim = (g->w * g->h) < size ? (g->w * g->h) : size;
        for (int i = 0; i < lim; i++) raster[i] = g->bitmap[i];
    }

    cache_insert(f, glyph, raster, size);
    return raster;
}

void nova_font_render(nova_machine *mc, const uint8_t *str, int len)
{
    nova_font *f = &mc->font;
    if (!f->run) { f->run_cap = 64; f->run = (uint8_t*)malloc(f->run_cap); }
    f->run_len = 0;

    uint8_t *base = f->run;
    for (int i = 0; i < len; i++) {
        uint8_t ch = str[i];
        if (f->run_len + 1 > f->run_cap) {
            f->run_cap *= 2;
            f->run = (uint8_t*)realloc(f->run, f->run_cap);
        }
        f->run[f->run_len++] = ch;
        if (f->run_len >= 2) {
            uint8_t prevc = base[f->run_len - 2];
            if (prevc == ch) f->run[f->run_len - 1] = 0;
        }
    }

    for (int k = 0; k < f->run_len; k++)
        (void)font_raster(mc, f->run[k], 0);
}
