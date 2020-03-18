#include "mantle.h"
#include "object.h"
#include "interp.h"
#include "arena.h"
#include "config.h"
#include "dvec.h"
#include <assert.h>


static Arena default_arena(10000000);

Arena &mantle_arena() {
    return default_arena;
}

void mantle_clear_arena() {
    default_arena.clear();
}

void _get_shape_verts(Shape *shape, vec3 *verts, int verts_count, vec3 trans) {
    static dvec _verts[128];
    shape_get_vertices(shape, verts_count, _verts);
    for (int i = 0; i < verts_count; ++i)
        verts[i] = vec3(trans.x, _verts[i].x + trans.y, _verts[i].y + trans.z);
}

void Mantle::generate_from_former_array(Arena &arena, Former *formers, int formers_count, vec3 obj_p) {
    assert(formers_count > 1);

    update_storage(arena, formers_count);

    /* fill in draw buffers data */

    vec3 *verts = tri_verts;

    /* former sections */

    for (int i = 0; i < formers_count; ++i) {
        Former *f = formers + i;
        vec3 p = obj_p;
        p.x += f->x;
        _get_shape_verts(&f->shape, verts, DRAW_VERTS_PER_POLY, p);
        verts += DRAW_VERTS_PER_POLY;
    }

    update_data();
}

void Mantle::generate_object_model(Arena &arena, Object *obj) {
    generate_from_former_array(arena, obj->formers, obj->formers_count, obj->p);
}
