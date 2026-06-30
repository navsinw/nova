#include "nova.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* a minimal recursive-descent JSON parser producing a flat node array.
   object members carry their key in node->str; array elements have empty keys.
   numbers are parsed as integers (this console has no floats). */

static int jnew(nova_json *j, int type)
{
    if (j->n >= j->cap) {
        int nc = j->cap ? j->cap * 2 : 16;
        nova_json_node *nn = (nova_json_node*)realloc(j->nodes, (size_t)nc * sizeof(nova_json_node));
        if (!nn) { j->err = 1; return -1; }
        j->nodes = nn; j->cap = nc;
    }
    int idx = j->n++;
    nova_json_node *node = &j->nodes[idx];
    node->type = type;
    node->num = 0;
    node->first_child = -1;
    node->next_sibling = -1;
    node->key[0] = 0;
    node->str[0] = 0;
    return idx;
}

static void skip_ws(nova_json *j)
{
    while (j->pos < j->len && isspace((unsigned char)j->src[j->pos])) j->pos++;
}

static int parse_value(nova_json *j);

static int parse_string_into(nova_json *j, char *dst, int cap)
{
    if (j->pos >= j->len || j->src[j->pos] != '"') { j->err = 1; return -1; }
    j->pos++;
    int o = 0;
    while (j->pos < j->len && j->src[j->pos] != '"') {
        char c = j->src[j->pos++];
        if (c == '\\' && j->pos < j->len) {
            char e = j->src[j->pos++];
            switch (e) { case 'n': c = '\n'; break; case 't': c = '\t'; break;
                         case 'r': c = '\r'; break; default: c = e; break; }
        }
        if (o < cap - 1) dst[o++] = c;
    }
    if (j->pos >= j->len) { j->err = 1; return -1; }
    j->pos++; /* closing quote */
    dst[o] = 0;
    return 0;
}

static int parse_object(nova_json *j)
{
    int obj = jnew(j, NJSON_OBJ);
    if (obj < 0) return -1;
    j->pos++; /* { */
    skip_ws(j);
    int last = -1;
    if (j->pos < j->len && j->src[j->pos] == '}') { j->pos++; return obj; }
    while (j->pos < j->len && !j->err) {
        skip_ws(j);
        char key[48];
        if (parse_string_into(j, key, sizeof(key)) != 0) return -1;
        skip_ws(j);
        if (j->pos >= j->len || j->src[j->pos] != ':') { j->err = 1; return -1; }
        j->pos++;
        int child = parse_value(j);
        if (child < 0) return -1;
        strncpy(j->nodes[child].key, key, sizeof(j->nodes[child].key) - 1);
        j->nodes[child].key[sizeof(j->nodes[child].key) - 1] = 0;
        if (last < 0) j->nodes[obj].first_child = child;
        else j->nodes[last].next_sibling = child;
        last = child;
        skip_ws(j);
        if (j->pos < j->len && j->src[j->pos] == ',') { j->pos++; continue; }
        if (j->pos < j->len && j->src[j->pos] == '}') { j->pos++; break; }
        j->err = 1; return -1;
    }
    return obj;
}

static int parse_array(nova_json *j)
{
    int arr = jnew(j, NJSON_ARR);
    if (arr < 0) return -1;
    j->pos++; /* [ */
    skip_ws(j);
    int last = -1;
    if (j->pos < j->len && j->src[j->pos] == ']') { j->pos++; return arr; }
    while (j->pos < j->len && !j->err) {
        int child = parse_value(j);
        if (child < 0) return -1;
        if (last < 0) j->nodes[arr].first_child = child;
        else j->nodes[last].next_sibling = child;
        last = child;
        skip_ws(j);
        if (j->pos < j->len && j->src[j->pos] == ',') { j->pos++; continue; }
        if (j->pos < j->len && j->src[j->pos] == ']') { j->pos++; break; }
        j->err = 1; return -1;
    }
    return arr;
}

static int parse_value(nova_json *j)
{
    skip_ws(j);
    if (j->pos >= j->len) { j->err = 1; return -1; }
    char c = j->src[j->pos];
    if (c == '{') return parse_object(j);
    if (c == '[') return parse_array(j);
    if (c == '"') {
        int n = jnew(j, NJSON_STR);
        if (n < 0) return -1;
        return parse_string_into(j, j->nodes[n].str, sizeof(j->nodes[n].str)) == 0 ? n : -1;
    }
    if (c == '-' || isdigit((unsigned char)c)) {
        int n = jnew(j, NJSON_NUM);
        if (n < 0) return -1;
        char *end = NULL;
        j->nodes[n].num = strtol(j->src + j->pos, &end, 10);
        if (end) j->pos = (int)(end - j->src);
        return n;
    }
    if (j->len - j->pos >= 4 && strncmp(j->src + j->pos, "true", 4) == 0) {
        int n = jnew(j, NJSON_BOOL); if (n < 0) return -1; j->nodes[n].num = 1; j->pos += 4; return n;
    }
    if (j->len - j->pos >= 5 && strncmp(j->src + j->pos, "false", 5) == 0) {
        int n = jnew(j, NJSON_BOOL); if (n < 0) return -1; j->nodes[n].num = 0; j->pos += 5; return n;
    }
    if (j->len - j->pos >= 4 && strncmp(j->src + j->pos, "null", 4) == 0) {
        int n = jnew(j, NJSON_NULL); if (n < 0) return -1; j->pos += 4; return n;
    }
    j->err = 1;
    return -1;
}

int nova_json_parse(nova_json *j, const char *text, int len)
{
    j->nodes = NULL; j->n = j->cap = 0;
    j->src = text; j->pos = 0; j->len = len; j->err = 0;
    int root = parse_value(j);
    if (j->err || root < 0) return -1;
    return root;
}

int nova_json_get(nova_json *j, int obj, const char *key)
{
    if (obj < 0 || obj >= j->n) return -1;
    if (j->nodes[obj].type != NJSON_OBJ) return -1;
    int c = j->nodes[obj].first_child;
    while (c >= 0) {
        if (strcmp(j->nodes[c].key, key) == 0) return c;
        c = j->nodes[c].next_sibling;
    }
    return -1;
}

void nova_json_free(nova_json *j)
{
    free(j->nodes);
    j->nodes = NULL;
    j->n = j->cap = 0;
}
