#include "nova.h"
#include <stdlib.h>
#include <string.h>

/* a point quadtree with a per-node capacity; subdivides up to maxdepth. */

#define QT_NODE_CAP 8

struct nova_qnode {
    int x, y, w, h;
    int depth;
    int px[QT_NODE_CAP], py[QT_NODE_CAP], pid[QT_NODE_CAP];
    int npts;
    struct nova_qnode *kids[4];
};

static struct nova_qnode *node_new(int x, int y, int w, int h, int depth)
{
    struct nova_qnode *n = (struct nova_qnode*)calloc(1, sizeof(*n));
    if (!n) return NULL;
    n->x = x; n->y = y; n->w = w; n->h = h; n->depth = depth;
    return n;
}

int nova_qt_init(nova_quadtree *q, int x, int y, int w, int h, int maxdepth)
{
    q->maxdepth = maxdepth > 0 ? maxdepth : 1;
    if (q->maxdepth > 12) q->maxdepth = 12;
    q->count = 0;
    q->root = node_new(x, y, w > 0 ? w : 1, h > 0 ? h : 1, 0);
    return q->root ? 0 : -1;
}

static int contains(struct nova_qnode *n, int x, int y)
{
    return x >= n->x && x < n->x + n->w && y >= n->y && y < n->y + n->h;
}

static int insert_node(struct nova_qnode *n, int x, int y, int id, int maxdepth)
{
    if (!contains(n, x, y)) return 0;
    if (n->npts < QT_NODE_CAP || n->depth >= maxdepth) {
        if (n->npts < QT_NODE_CAP) {
            n->px[n->npts] = x; n->py[n->npts] = y; n->pid[n->npts] = id;
            n->npts++;
            return 1;
        }
        return 0; /* full leaf at max depth: drop */
    }
    if (!n->kids[0]) {
        int hw = n->w / 2, hh = n->h / 2;
        if (hw < 1) hw = 1; if (hh < 1) hh = 1;
        n->kids[0] = node_new(n->x, n->y, hw, hh, n->depth + 1);
        n->kids[1] = node_new(n->x + hw, n->y, n->w - hw, hh, n->depth + 1);
        n->kids[2] = node_new(n->x, n->y + hh, hw, n->h - hh, n->depth + 1);
        n->kids[3] = node_new(n->x + hw, n->y + hh, n->w - hw, n->h - hh, n->depth + 1);
    }
    for (int k = 0; k < 4; k++)
        if (n->kids[k] && insert_node(n->kids[k], x, y, id, maxdepth)) return 1;
    /* fallback: keep in this node */
    if (n->npts < QT_NODE_CAP) {
        n->px[n->npts] = x; n->py[n->npts] = y; n->pid[n->npts] = id; n->npts++;
        return 1;
    }
    return 0;
}

int nova_qt_insert(nova_quadtree *q, int x, int y, int id)
{
    if (!q->root) return -1;
    if (insert_node(q->root, x, y, id, q->maxdepth)) { q->count++; return 0; }
    return -1;
}

static int rect_overlap(struct nova_qnode *n, int x, int y, int w, int h)
{
    return n->x < x + w && n->x + n->w > x && n->y < y + h && n->y + n->h > y;
}

static int query_node(struct nova_qnode *n, int x, int y, int w, int h, int *out, int outcap, int found)
{
    if (!n || !rect_overlap(n, x, y, w, h)) return found;
    for (int i = 0; i < n->npts; i++) {
        if (n->px[i] >= x && n->px[i] < x + w && n->py[i] >= y && n->py[i] < y + h) {
            if (found < outcap) out[found] = n->pid[i];
            found++;
        }
    }
    for (int k = 0; k < 4; k++)
        found = query_node(n->kids[k], x, y, w, h, out, outcap, found);
    return found;
}

int nova_qt_query(nova_quadtree *q, int x, int y, int w, int h, int *out, int outcap)
{
    if (!q->root) return 0;
    return query_node(q->root, x, y, w, h, out, outcap, 0);
}

static void free_node(struct nova_qnode *n)
{
    if (!n) return;
    for (int k = 0; k < 4; k++) free_node(n->kids[k]);
    free(n);
}

void nova_qt_free(nova_quadtree *q)
{
    free_node(q->root);
    q->root = NULL;
    q->count = 0;
}
