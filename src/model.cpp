#include "model.h"
#include "object.h"
#include "part.h"
#include "debug.h"
#include <math.h>
#include <assert.h>


Model::Model() : objects_count(0), wings_count(0) {}

Model::~Model() {
    model_clear(this);
}

void model_clear(Model *m) {
    for (int i = 0; i < m->objects_count; ++i)
        delete m->objects[i];
    m->objects_count = 0;
    for (int i = 0; i < m->wings_count; ++i)
        delete m->wings[i];
    m->wings_count = 0;
}

void model_add_object(Model *m, Object *o) {
    break_assert(m->objects_count + m->wings_count < MAX_ELEMS);
    m->objects[m->objects_count++] = o;
}

void model_add_wing(Model *m, Wing *w) {
    break_assert(m->objects_count + m->wings_count < MAX_ELEMS);
    m->wings[m->wings_count++] = w;
}

void Model::deselect_all() {
    for (int i = 0; i < objects_count; ++i) {
        objects[i]->selected = false;
        objects[i]->deselect_all_handles();
    }
}

bool Model::move_selected(vec3 move_xyz, vec3 target_yz) {
    bool requires_reloft = false;

    for (int i = 0; i < objects_count; ++i) {
        Object *obj = objects[i];

        /* move objects */

        if (obj->selected)
            obj->move(move_xyz);

        /* move skin former handles */

        // for (int j = 0; j < 2; ++j) {

        //     SkinFormer *former = (j == 0) ? obj->tail_skin_former : obj->nose_skin_former;
        //     SkinPoint *proto_points = (j == 0) ? obj->part.tail_points : obj->part.nose_points;

        //     for (int k = 0; k < SHAPE_CURVES; ++k) {
        //         Handle &handle = obj->handles[j][k];
        //         SkinPoint &proto_point = proto_points[k];

        //         if (!handle.selected)
        //             continue;

        //         ShapePoint &former_point = former->shape.points[k];

        //         /* move handle */

        //         vec2 old_p = former_point.p;
        //         former_point.p = vec2(target_yz.y, target_yz.z) - obj->p.yz();

        //         /* degrees of freedom constraints */

        //         if (!proto_point.move_x)
        //             former_point.p.x = old_p.x;
        //         if (!proto_point.move_y)
        //             former_point.p.y = old_p.y;

        //         requires_reloft = true;
        //     }
        // }
    }

    return requires_reloft;
}

bool Model::delete_selected() {
    bool requires_reloft = false;

    /* remove selected objects */

    for (int i = 0; i < objects_count;) {
        Object *o = objects[i];

        if (o->selected) {
            --objects_count;
            for (int j = i; j < objects_count; ++j)
                objects[j] = objects[j + 1];
            requires_reloft = true;
        }
        else
            ++i;
    }

    return requires_reloft;
}
