#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* growable interned string table used by the tools and the META parser. */

void nova_strtab_init(nova_strtab *st)
{
    st->items = NULL;
    st->n = 0;
    st->cap = 0;
}

static int ensure(nova_strtab *st, int need)
{
    if (need <= st->cap) return 0;
    int nc = st->cap ? st->cap * 2 : 8;
    while (nc < need) nc *= 2;
    char **ni = (char**)realloc(st->items, (size_t)nc * sizeof(char*));
    if (!ni) return -1;
    st->items = ni;
    st->cap = nc;
    return 0;
}

int nova_strtab_find(nova_strtab *st, const char *s)
{
    for (int i = 0; i < st->n; i++)
        if (st->items[i] && strcmp(st->items[i], s) == 0)
            return i;
    return -1;
}

int nova_strtab_add(nova_strtab *st, const char *s)
{
    int existing = nova_strtab_find(st, s);
    if (existing >= 0) return existing;
    if (ensure(st, st->n + 1) != 0) return -1;
    size_t len = strlen(s);
    char *copy = (char*)malloc(len + 1);
    if (!copy) return -1;
    memcpy(copy, s, len + 1);
    st->items[st->n] = copy;
    return st->n++;
}

const char *nova_strtab_get(nova_strtab *st, int idx)
{
    if (idx < 0 || idx >= st->n) return NULL;
    return st->items[idx];
}

void nova_strtab_free(nova_strtab *st)
{
    for (int i = 0; i < st->n; i++)
        free(st->items[i]);
    free(st->items);
    st->items = NULL;
    st->n = st->cap = 0;
}
