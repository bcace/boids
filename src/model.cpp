#include "model.h"
#include "object.h"
#include "part.h"
#include <math.h>
#include <assert.h>


Model::~Model() {
    clear();
}

void Model::clear() {
    for (int i = 0; i < objects.count; ++i)
        delete objects[i];
    objects.clear();
}

void Model::add_object(Object *o) {
    objects.add(o);
}

void Model::deselect_all() {
    for (int i = 0; i < objects.count; ++i) {
        objects[i]->selected = false;
        objects[i]->deselect_all_handles();
    }
}

bool Model::move_selected(vec3 move_xyz, vec3 target_yz) {
    bool requires_reloft = false;

    for (int i = 0; i < objects.count; ++i) {
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

    for (int i = 0; i < objects.count;) {
        Object *o = objects[i];

        if (o->selected) {
            objects.remove_at(i);
            requires_reloft = true;
        }
        else
            ++i;
    }

    return requires_reloft;
}
