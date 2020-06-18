#include "ui_mantle.h"
#include "ui_graphics.h"
#include "modeling_airfoil.h"
#include "modeling_shape.h"
#include "modeling_wing.h"
#include "memory_arena.h"
#include "math_vec.h"
#include "math_dvec.h"
#include <assert.h>
#include <math.h>

#define DRAW_VERTS_PER_POINT    16
#define DRAW_VERTS_PER_POLY     (4 * DRAW_VERTS_PER_POINT)


static Arena default_arena(10000000);

Mantle::Mantle() : verts(0), verts_count(0),
                   indices(0), indices_count(0),
                   sections_count(0), verts_per_section(0) {}

void mantle_draw_quads(Mantle *m, ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color) {
    program.set_uniform_vec4(2, color);
    program.set_data<vec3>(0, m->verts_count, m->verts);
    graph_draw_quads_indexed(m->indices_count, m->indices);
}


Arena &mantle_arena() {
    return default_arena;
}

void mantle_clear_arena() {
    default_arena.clear();
}

static void _get_shape_verts(Shape *shape, vec3 *verts, int verts_count, float dx, float dy, float dz) {
    static dvec _verts[128];
    shape_get_vertices(shape, verts_count, _verts);
    for (int i = 0; i < verts_count; ++i)
        verts[i] = vec3(dx, (float)_verts[i].x + dy, (float)_verts[i].y + dz);
}

static void _update_storage(Mantle *m, Arena *arena, int sections_count, int verts_per_section) {
    m->sections_count = sections_count;
    m->verts_per_section = verts_per_section;

    m->verts_count = sections_count * verts_per_section;
    m->indices_count = (sections_count - 1) * verts_per_section * 4;

    m->verts = arena->alloc<vec3>(m->verts_count);
    m->indices = arena->alloc<int>(m->indices_count);
}

static void _update_data(Mantle *m) {

    int *idx = m->indices;

    for (int i2 = 1; i2 < m->sections_count; ++i2) {
        int i1 = i2 - 1;

        for (int j1 = 0; j1 < m->verts_per_section; ++j1) {
            int j2 = (j1 + 1) % m->verts_per_section;

            *idx++ = i1 * m->verts_per_section + j1;
            *idx++ = i1 * m->verts_per_section + j2;
            *idx++ = i2 * m->verts_per_section + j2;
            *idx++ = i2 * m->verts_per_section + j1;
        }
    }
}

void mantle_generate_from_former_array(Mantle *m, Arena *arena, Former *formers, int formers_count, float x, float y, float z) {
    _update_storage(m, arena, formers_count, DRAW_VERTS_PER_POLY);

    vec3 *section_verts = m->verts;
    for (int i = 0; i < formers_count; ++i) {
        Former *f = formers + i;
        _get_shape_verts(&f->shape, section_verts, m->verts_per_section, x + f->x, y, z);
        section_verts += m->verts_per_section;
    }

    _update_data(m);
}

void mantle_generate_from_wing_formers(Mantle *m, Arena *arena, WFormer *r_f, WFormer *t_f, float x, float y, float z) {
    _update_storage(m, arena, 2, AIRFOIL_POINTS);

    double dihedral = atan2(t_f->z - r_f->z, t_f->y - r_f->y);

    airfoil_get_points(&r_f->airfoil, m->verts, dihedral,
                       r_f->chord, r_f->aoa,
                       r_f->x + x, r_f->y + y, r_f->z + z);
    airfoil_get_points(&t_f->airfoil, m->verts + AIRFOIL_POINTS, dihedral,
                       t_f->chord, t_f->aoa,
                       t_f->x + x, t_f->y + y, t_f->z + z);

    _update_data(m);
}
