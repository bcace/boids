#include "modeling_object.h"
#include <float.h>


void object_finish(Object *o) {
    o->selected = false;
    object_reset_drag_p(o);
    object_update_extents(o);
}

void object_update_extents(Object *o) {
    o->min_x = o->p.x + o->def.t_skin_former.x;
    o->max_x = o->p.x + o->def.n_skin_former.x;
    float min_y = FLT_MAX;
    float max_y = -FLT_MAX;
    float min_z = FLT_MAX;
    float max_z = -FLT_MAX;
    for (int i = 0; i < o->def.formers_count; ++i) {
        Curve *curves = o->def.formers[i].shape.curves;
        if (curves[2].x < min_y)
            min_y = curves[2].x;
        if (curves[0].x > max_y)
            max_y = curves[0].x;
        if (curves[3].y < min_z)
            min_z = curves[3].y;
        if (curves[1].y > max_z)
            max_z = curves[1].y;
    }
    o->min_y = o->p.y + min_y;
    o->max_y = o->p.y + max_y;
    o->min_z = o->p.z + min_z;
    o->max_z = o->p.z + max_z;
}

bool object_should_be_centered(Object *o) {
    return o->p.y < (o->max_y - o->min_y) * 0.5f;
}

bool object_should_be_mirrored(Object *o) {
    return !object_should_be_centered(o);
}

void object_move(Object *o, vec3 dp) {
    o->drag_p += dp;
}

void object_reset_drag_p(Object *o) {
    o->drag_p = o->p;
}
