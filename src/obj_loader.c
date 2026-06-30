#include "nova.h"
#include <stdlib.h>
#include <ctype.h>

/* a trimmed OBJ-like text loader: 'v x y z' vertices and 'e a b' edges
   (1-based indices, like OBJ faces). fills a nova_mesh. */

static const char *skip_line(const char *p, const char *end)
{
    while (p < end && *p != '\n') p++;
    return (p < end) ? p + 1 : end;
}

int nova_obj_load(nova_mesh *me, const char *text, int len)
{
    me->nverts = 0;
    me->nedges = 0;
    me->angle_x = me->angle_y = me->angle_z = 0;
    const char *p = text;
    const char *end = text + len;

    while (p < end) {
        while (p < end && (*p == ' ' || *p == '\t')) p++;
        if (p >= end) break;
        char tag = *p;
        if (tag == 'v') {
            p++;
            char *e2;
            long x = strtol(p, &e2, 10); p = e2;
            long y = strtol(p, &e2, 10); p = e2;
            long z = strtol(p, &e2, 10); p = e2;
            if (me->nverts < NOVA_MESH_MAXV) {
                me->verts[me->nverts].x = (int32_t)x << 8;
                me->verts[me->nverts].y = (int32_t)y << 8;
                me->verts[me->nverts].z = (int32_t)z << 8;
                me->nverts++;
            }
        } else if (tag == 'e') {
            p++;
            char *e2;
            long a = strtol(p, &e2, 10); p = e2;
            long b = strtol(p, &e2, 10); p = e2;
            a--; b--; /* 1-based -> 0-based */
            if (me->nedges < NOVA_MESH_MAXE && a >= 0 && b >= 0 &&
                a < me->nverts && b < me->nverts) {
                me->edges[me->nedges][0] = (int)a;
                me->edges[me->nedges][1] = (int)b;
                me->nedges++;
            }
        }
        p = skip_line(p, end);
    }
    return me->nverts > 0 ? 0 : -1;
}
