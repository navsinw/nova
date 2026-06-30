#include "nova.h"

/* wireframe 3D: rotate a vertex set, project with a fixed camera distance,
   and draw edges via the rasterizer. mesh data is bounds-checked at load. */

static uint16_t m16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

int nova_mesh_load(nova_mesh *me, const uint8_t *data, uint32_t len)
{
    me->nverts = 0; me->nedges = 0;
    me->angle_x = me->angle_y = me->angle_z = 0;
    if (!data || len < 4) return -1;
    int nv = m16(data);
    int ne = m16(data + 2);
    if (nv < 0 || nv > NOVA_MESH_MAXV || ne < 0 || ne > NOVA_MESH_MAXE) return -1;
    uint32_t off = 4;
    if (off + (uint32_t)nv * 6u > len) return -1;
    for (int i = 0; i < nv; i++) {
        me->verts[i].x = (int16_t)m16(data + off) << 8;
        me->verts[i].y = (int16_t)m16(data + off + 2) << 8;
        me->verts[i].z = (int16_t)m16(data + off + 4) << 8;
        off += 6;
    }
    me->nverts = nv;
    if (off + (uint32_t)ne * 4u > len) return -1;
    for (int i = 0; i < ne; i++) {
        int a = m16(data + off), b = m16(data + off + 2);
        off += 4;
        if (a >= nv || b >= nv) continue;
        me->edges[me->nedges][0] = a;
        me->edges[me->nedges][1] = b;
        me->nedges++;
    }
    return 0;
}

void nova_mesh_cube(nova_mesh *me, int32_t s)
{
    static const int v[8][3] = {
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
        {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
    };
    static const int e[12][2] = {
        {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
    };
    me->nverts = 8;
    for (int i = 0; i < 8; i++) {
        me->verts[i].x = v[i][0] * s;
        me->verts[i].y = v[i][1] * s;
        me->verts[i].z = v[i][2] * s;
    }
    me->nedges = 12;
    for (int i = 0; i < 12; i++) { me->edges[i][0] = e[i][0]; me->edges[i][1] = e[i][1]; }
    me->angle_x = me->angle_y = me->angle_z = 0;
}

static void project(nova_mesh *me, int i, int cx, int cy, int *px, int *py)
{
    nova_vec3 p = me->verts[i];
    nova_mat3 rz = nova_mat3_rotz(me->angle_z);
    nova_mat3 ry = nova_mat3_rotz(me->angle_y);
    p = nova_mat3_apply(rz, p);
    /* fake y rotation by swapping/scaling z into x */
    int32_t nx = nova_fp_mul(p.x, nova_fp_cos(me->angle_y << 8)) - nova_fp_mul(p.z, nova_fp_sin(me->angle_y << 8));
    int32_t nz = nova_fp_mul(p.x, nova_fp_sin(me->angle_y << 8)) + nova_fp_mul(p.z, nova_fp_cos(me->angle_y << 8));
    p.x = nx; p.z = nz;
    (void)ry;
    int32_t dist = 4 * NOVA_FP_ONE;
    int32_t denom = dist + p.z;
    if (denom < NOVA_FP_ONE / 4) denom = NOVA_FP_ONE / 4;
    int32_t f = nova_fp_div(2 * NOVA_FP_ONE, denom);
    *px = cx + (nova_fp_mul(p.x, f) >> 16) * 20 / 1;
    *py = cy + (nova_fp_mul(p.y, f) >> 16) * 20 / 1;
}

void nova_mesh_draw(nova_machine *mc, nova_mesh *me, int cx, int cy, uint8_t color)
{
    for (int i = 0; i < me->nedges; i++) {
        int a = me->edges[i][0], b = me->edges[i][1];
        if (a < 0 || b < 0 || a >= me->nverts || b >= me->nverts) continue;
        int ax, ay, bx, by;
        project(me, a, cx, cy, &ax, &ay);
        project(me, b, cx, cy, &bx, &by);
        nova_raster_line(mc, ax, ay, bx, by, color);
    }
}
