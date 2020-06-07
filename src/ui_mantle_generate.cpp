#include "ui_mantle.h"
#include "modeling_object.h"
#include "modeling_wing.h"
#include "memory_arena.h"
#include "modeling_config.h"
#include "math_dvec.h"
#include "math_vec.h"
#include <assert.h>


static Arena default_arena(10000000);

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

static void _init_mantle(Mantle *m, int sections_count, int verts_per_section, bool draw_caps) {
    m->sections_count = sections_count;
    m->verts_per_section = verts_per_section;
    m->is_wing = draw_caps;
}

static void _mantle_update_storage(Mantle *m, Arena *arena) {
    m->verts = arena->alloc<vec3>(m->verts_count);
    m->indices = arena->alloc<int>(m->indices_count);
}

static void _update_storage(Mantle *m, Arena *arena,
                            int sections_count, int verts_per_section, bool draw_caps) {

    m->sections_count = sections_count;
    m->verts_per_section = verts_per_section;
    m->is_wing = draw_caps;

    m->verts_count = sections_count * verts_per_section;
    m->indices_count = (sections_count - 1) * verts_per_section * 4;

    if (draw_caps)
        m->indices_count += AIRFOIL_X_SUBDIVS * 2 * 4;

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

    if (m->is_wing) {
        bool odd_verts_per_section = (m->verts_per_section % 2) == 1;

        int verts_per_side = odd_verts_per_section ?
                             (m->verts_per_section - 1) / 2 :
                             m->verts_per_section / 2;

        for (int i = 1; i < verts_per_side; ++i) {
            int i1 = i - 1;
            int i2 = i;
            int i3 = m->verts_per_section - i - 1;
            int i4 = m->verts_per_section - i;

            *idx++ = i1;
            *idx++ = i2;
            *idx++ = i3;
            *idx++ = i4;

            *idx++ = i1 + m->verts_per_section;
            *idx++ = i2 + m->verts_per_section;
            *idx++ = i3 + m->verts_per_section;
            *idx++ = i4 + m->verts_per_section;
        }

        if (odd_verts_per_section) {
            *idx++ = verts_per_side - 1;
            *idx++ = verts_per_side;
            *idx++ = verts_per_side;
            *idx++ = verts_per_side + 1;

            *idx++ = verts_per_side - 1 + m->verts_per_section;
            *idx++ = verts_per_side + m->verts_per_section;
            *idx++ = verts_per_side + m->verts_per_section;
            *idx++ = verts_per_side + 1 + m->verts_per_section;
        }
    }
}

void mantle_generate_from_former_array(Mantle *m, Arena *arena, Former *formers, int formers_count, float x, float y, float z) {
    assert(formers_count > 1);

    _update_storage(m, arena, formers_count, DRAW_VERTS_PER_POLY, false);

    vec3 *section_verts = m->verts;
    for (int i = 0; i < formers_count; ++i) {
        Former *f = formers + i;
        _get_shape_verts(&f->shape, section_verts, m->verts_per_section, x + f->x, y, z);
        section_verts += m->verts_per_section;
    }

    _update_data(m);
}

/* Used to generate a draggable representation of a wing root. */
void mantle_generate_from_airfoil(Mantle *m, Arena *arena, Airfoil *airfoil,
                                  float x, float y, float z) {

    _update_storage(m, arena, 2, AIRFOIL_POINTS, true);

    static tvec r_verts[AIRFOIL_POINTS];
    static tvec t_verts[AIRFOIL_POINTS];
    airfoil_get_points(airfoil, r_verts, 1.0, -WING_SNAP_WIDTH, 0.0, 0.0, x, y, z);
    airfoil_get_points(airfoil, t_verts, 1.0,  WING_SNAP_WIDTH, 0.0, 0.0, x, y, z);

    vec3 *v1 = m->verts;
    vec3 *v2 = v1 + AIRFOIL_POINTS;
    for (int i = 0; i < AIRFOIL_POINTS; ++i) {
        v1->x = r_verts[i].x;
        v1->y = r_verts[i].y;
        v1->z = r_verts[i].z;
        v2->x = t_verts[i].x;
        v2->y = t_verts[i].y;
        v2->z = t_verts[i].z;
        ++v1;
        ++v2;
    }

    _update_data(m);
}