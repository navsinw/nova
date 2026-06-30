#include "nova.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* INI-style config: 'key = value' lines, ';' or '#' comments, optional
   '[section]' headers folded into 'section.key'. parallel arrays keep the
   key/value pairing exact (no interning). */

void nova_config_init(nova_config *c)
{
    c->keys = NULL;
    c->vals = NULL;
    c->n = c->cap = 0;
}

static char *dup_str(const char *s)
{
    size_t n = strlen(s);
    char *p = (char*)malloc(n + 1);
    if (p) memcpy(p, s, n + 1);
    return p;
}

static int cfg_put(nova_config *c, const char *key, const char *val)
{
    if (c->n >= c->cap) {
        int nc = c->cap ? c->cap * 2 : 16;
        char **nk = (char**)realloc(c->keys, (size_t)nc * sizeof(char*));
        char **nv = (char**)realloc(c->vals, (size_t)nc * sizeof(char*));
        if (!nk || !nv) { free(nk); free(nv); return -1; }
        c->keys = nk; c->vals = nv; c->cap = nc;
    }
    c->keys[c->n] = dup_str(key);
    c->vals[c->n] = dup_str(val);
    c->n++;
    return 0;
}

static char *rtrim(char *s)
{
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

static char *ltrim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

int nova_config_parse(nova_config *c, const char *text)
{
    char section[64] = {0};
    char line[256];
    const char *p = text;
    int count = 0;

    while (*p) {
        int n = 0;
        while (*p && *p != '\n' && n < 255) line[n++] = *p++;
        line[n] = 0;
        if (*p == '\n') p++;

        char *s = ltrim(line);
        char *cm = strpbrk(s, ";#");
        if (cm) *cm = 0;
        s = rtrim(s);
        if (!*s) continue;

        if (s[0] == '[') {
            char *end = strchr(s, ']');
            if (end) { *end = 0; strncpy(section, s + 1, 63); section[63] = 0; }
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = rtrim(ltrim(s));
        char *val = rtrim(ltrim(eq + 1));

        char full[160];
        if (section[0]) snprintf(full, sizeof(full), "%s.%s", section, key);
        else            snprintf(full, sizeof(full), "%s", key);

        if (cfg_put(c, full, val) == 0) count++;
    }
    return count;
}

const char *nova_config_get(nova_config *c, const char *key)
{
    for (int i = 0; i < c->n; i++)
        if (strcmp(c->keys[i], key) == 0) return c->vals[i];
    return NULL;
}

int nova_config_get_int(nova_config *c, const char *key, int def)
{
    const char *v = nova_config_get(c, key);
    if (!v) return def;
    return (int)strtol(v, NULL, 0);
}

void nova_config_free(nova_config *c)
{
    for (int i = 0; i < c->n; i++) { free(c->keys[i]); free(c->vals[i]); }
    free(c->keys); free(c->vals);
    c->keys = c->vals = NULL;
    c->n = c->cap = 0;
}
