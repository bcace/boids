#include "object.h"
#include "part.h"
#include "interp.h"
#include <math.h>
#include <float.h>
#include <assert.h>


void object_finish(Object *o) {
    o->selected = false;
    o->drag_p = o->p;
    object_update_extents(o);
}

void object_update_extents(Object *o) {
    o->min_x = o->p.x + o->tail_skin_former.x;
    o->max_x = o->p.x + o->nose_skin_former.x;
    float min_y = FLT_MAX;
    float max_y = -FLT_MAX;
    float min_z = FLT_MAX;
    float max_z = -FLT_MAX;
    for (int i = 0; i < o->formers_count; ++i) {
        Curve *curves = o->formers[i].shape.curves;
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

void Object::move(vec3 dp) {
    drag_p += dp;
}

void Object::reset_drag_p() {
    drag_p = p;
}

void Object::deselect_all_handles() {
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < SHAPE_CURVES; ++j)
            handles[i][j].selected = false;
}

void object_update_mantle(Object *o) {
    o->model_mantle.generate_object_model(mantle_arena(), o);
}
