#include "nova.h"

/* 16.16 fixed-point 3x3 matrices for 2D affine and small 3D rotations. */

void nova_mat3_identity(nova_mat3 *m)
{
    for (int i = 0; i < 9; i++) m->m[i] = 0;
    m->m[0] = m->m[4] = m->m[8] = NOVA_FP_ONE;
}

nova_mat3 nova_mat3_mul(nova_mat3 a, nova_mat3 b)
{
    nova_mat3 r;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int32_t acc = 0;
            for (int k = 0; k < 3; k++)
                acc += nova_fp_mul(a.m[row * 3 + k], b.m[k * 3 + col]);
            r.m[row * 3 + col] = acc;
        }
    }
    return r;
}

nova_mat3 nova_mat3_rotz(int angle)
{
    nova_mat3 m;
    nova_mat3_identity(&m);
    int32_t c = nova_fp_cos(angle << 8);
    int32_t s = nova_fp_sin(angle << 8);
    m.m[0] = c;  m.m[1] = -s;
    m.m[3] = s;  m.m[4] = c;
    return m;
}

nova_mat3 nova_mat3_scale(int32_t sx, int32_t sy)
{
    nova_mat3 m;
    nova_mat3_identity(&m);
    m.m[0] = sx;
    m.m[4] = sy;
    return m;
}

nova_mat3 nova_mat3_translate(int32_t tx, int32_t ty)
{
    nova_mat3 m;
    nova_mat3_identity(&m);
    m.m[2] = tx;
    m.m[5] = ty;
    return m;
}

nova_vec3 nova_mat3_apply(nova_mat3 m, nova_vec3 v)
{
    nova_vec3 r;
    r.x = nova_fp_mul(m.m[0], v.x) + nova_fp_mul(m.m[1], v.y) + nova_fp_mul(m.m[2], v.z);
    r.y = nova_fp_mul(m.m[3], v.x) + nova_fp_mul(m.m[4], v.y) + nova_fp_mul(m.m[5], v.z);
    r.z = nova_fp_mul(m.m[6], v.x) + nova_fp_mul(m.m[7], v.y) + nova_fp_mul(m.m[8], v.z);
    return r;
}

nova_vec3 nova_vec3_add(nova_vec3 a, nova_vec3 b)
{
    nova_vec3 r = { a.x + b.x, a.y + b.y, a.z + b.z };
    return r;
}

int32_t nova_vec3_dot(nova_vec3 a, nova_vec3 b)
{
    return nova_fp_mul(a.x, b.x) + nova_fp_mul(a.y, b.y) + nova_fp_mul(a.z, b.z);
}
