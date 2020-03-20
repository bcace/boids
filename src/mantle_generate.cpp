#include "mantle.h"
#include "object.h"
#include "wing.h"
#include "interp.h"
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

static void _get_shape_verts(Shape *shape, vec3 *verts, int verts_count, vec3 trans) {
    static dvec _verts[128];
    shape_get_vertices(shape, verts_count, _verts);
    for (int i = 0; i < verts_count; ++i)
        verts[i] = vec3(trans.x, _verts[i].x + trans.y, _verts[i].y + trans.z);
}

static void _init_mantle(Mantle *m, int sections_count, int verts_per_section, bool draw_caps) {
    m->sections_count = sections_count;
    m->verts_per_section = verts_per_section;
    m->draw_caps = draw_caps;
}

static void _mantle_update_storage(Mantle *m, Arena *arena) {
    m->verts = arena->alloc<vec3>(m->verts_count);
    m->indices = arena->alloc<int>(m->indices_count);
}

// TODO: make this into a procedure.
void Mantle::generate_from_former_array(Arena &arena, Former *formers, int formers_count, vec3 obj_p) {
    assert(formers_count > 1);

    _init_mantle(this, formers_count, DRAW_VERTS_PER_POLY, false);

    verts_count = sections_count * verts_per_section;
    indices_count = (sections_count - 1) * verts_per_section * 4;

    _mantle_update_storage(this, &arena);

    vec3 *section_verts = verts;
    for (int i = 0; i < formers_count; ++i) {
        Former *f = formers + i;
        vec3 p = obj_p;
        p.x += f->x;
        _get_shape_verts(&f->shape, section_verts, verts_per_section, p);
        section_verts += verts_per_section;
    }

    update_data();
}

void Mantle::generate_object_model(Arena &arena, Object *obj) {
    generate_from_former_array(arena, obj->formers, obj->formers_count, obj->p);
}

/* Used to generate a draggable representation of a wing root. */
void mantle_generate_from_airfoil(Mantle *m, Arena *arena, Airfoil *airfoil,
                                  float x, float y, float z) {

    _init_mantle(m, 2, AIRFOIL_POINTS, true);

    m->verts_count = m->sections_count * m->verts_per_section;
    m->indices_count = ((m->sections_count - 1) * m->verts_per_section +
                        (AIRFOIL_X_SUBDIVS * 2)) * 4;

    _mantle_update_storage(m, arena);

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

    m->update_data();
}

void Mantle::update_data() {

    int *idx = indices;

    for (int i2 = 1; i2 < sections_count; ++i2) {
        int i1 = i2 - 1;

        for (int j1 = 0; j1 < verts_per_section; ++j1) {
            int j2 = (j1 + 1) % verts_per_section;

            *idx++ = i1 * verts_per_section + j1;
            *idx++ = i1 * verts_per_section + j2;
            *idx++ = i2 * verts_per_section + j2;
            *idx++ = i2 * verts_per_section + j1;
        }
    }

    if (draw_caps) {
        bool odd_verts_per_section = (verts_per_section % 2) == 1;

        int verts_per_side = odd_verts_per_section ?
                             (verts_per_section - 1) / 2 :
                             verts_per_section / 2;

        for (int i = 1; i < verts_per_side; ++i) {
            int i1 = i - 1;
            int i2 = i;
            int i3 = verts_per_section - i - 1;
            int i4 = verts_per_section - i;

            *idx++ = i1;
            *idx++ = i2;
            *idx++ = i3;
            *idx++ = i4;

            *idx++ = i1 + verts_per_section;
            *idx++ = i2 + verts_per_section;
            *idx++ = i3 + verts_per_section;
            *idx++ = i4 + verts_per_section;
        }

        if (odd_verts_per_section) {
            *idx++ = verts_per_side - 1;
            *idx++ = verts_per_side;
            *idx++ = verts_per_side;
            *idx++ = verts_per_side + 1;

            *idx++ = verts_per_side - 1 + verts_per_section;
            *idx++ = verts_per_side + verts_per_section;
            *idx++ = verts_per_side + verts_per_section;
            *idx++ = verts_per_side + 1 + verts_per_section;
        }
    }
}
