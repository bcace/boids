#include "mantle.h"
#include "object.h"
#include "wing.h"
#include "arena.h"
#include "config.h"
#include "dvec.h"
#include "vec.h"
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
        verts[i] = vec3(dx, _verts[i].x + dy, _verts[i].y + dz);
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

    vec3 *section_verts = m->verts;
    for (int i = 0; i < AIRFOIL_POINTS; ++i) {
        dvec p = airfoil_get_point(airfoil, i);
        vec3 *v1 = section_verts + i;
        vec3 *v2 = v1 + AIRFOIL_POINTS;
        v1->x = v2->x = x - (float)p.x;
        v1->z = v2->z = z + (float)p.y;
        v1->y = y - (float)WING_SNAP_WIDTH * 0.5f;
        v2->y = y + (float)WING_SNAP_WIDTH * 0.5f;
    }

    _update_data(m);
}
