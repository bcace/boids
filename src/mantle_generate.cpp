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
        Former &f = formers[i];
        vec3 former_p = obj_p;
        former_p.x += f.x;
        _get_shape_verts(&f.shape, verts, DRAW_VERTS_PER_POLY, former_p);
        verts += DRAW_VERTS_PER_POLY;
    }

    update_data();
}

// TODO: remove!
void Mantle::generate_from_skin_formers(Arena &arena, SkinFormer *tail_skin_former, vec3 tail_obj_p, SkinFormer *nose_skin_former, vec3 nose_obj_p) {
    // vec3 tail_p[SHAPE_CURVES], tail_c[SHAPE_CURVES];
    // vec3 nose_p[SHAPE_CURVES], nose_c[SHAPE_CURVES];

    // float pipe_l = (nose_skin_former->x + nose_obj_p.x) - (tail_skin_former->x + tail_obj_p.x);
    // float c = pipe_l * LONGITUDINAL_SMOOTHNESS;

    // for (int i = 0; i < SHAPE_CURVES; ++i) {
    //     tail_p[i] = vec3(tail_skin_former->x, tail_skin_former->shape.points[i].p) + tail_obj_p;
    //     tail_c[i] = tail_p[i] + tail_skin_former->l_tangents[i] * c;
    //     nose_p[i] = vec3(nose_skin_former->x, nose_skin_former->shape.points[i].p) + nose_obj_p;
    //     nose_c[i] = nose_p[i] - nose_skin_former->l_tangents[i] * c;
    // }

    // int sections_count = 10;
    // update_storage(arena, sections_count);

    // float dt = 1.0f / (sections_count - 1);
    // float dx = pipe_l / (sections_count - 1);

    // vec3 *verts = tri_verts;

    // for (int i = 0; i < sections_count; ++i) {
    //     float t = i * dt;
    //     float smooth_t = SMOOTHSTEP(t);

    //     Shape shape;

    //     for (int j = 0; j < SHAPE_CURVES; ++j) {
    //         vec3 p = bezier(tail_p[j], tail_c[j], nose_c[j], nose_p[j], t);
    //         shape.curves[j].x = p.y;
    //         shape.curves[j].y = p.z;
    //         shape.points[j].w = lerp_1d(tail_skin_former->shape.points[j].w,
    //                                     nose_skin_former->shape.points[j].w,
    //                                     smooth_t);
    //     }

    //     if ((unsigned int)verts == 0xcccccccc) {
    //         int cond_break = 0;
    //     }

    //     _get_shape_verts(&shape, verts, DRAW_VERTS_PER_POLY, vec3(tail_skin_former->x + i * dx + tail_obj_p.x, 0, 0));
    //     verts += DRAW_VERTS_PER_POLY;
    // }

    // update_data();
}

void Mantle::generate_object_model(Arena &arena, Object *obj) {
    generate_from_former_array(arena, obj->formers, obj->formers_count, obj->p);
}

void Mantle::generate_object_skin(Arena &arena, Object *obj) {
    // generate_from_skin_formers(arena, obj->tail_skin_former, obj->p, obj->nose_skin_former, obj->p);
}
