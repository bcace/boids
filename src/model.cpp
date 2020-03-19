#include "model.h"
#include "object.h"
#include "wing.h"
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
    for (int i = 0; i < wings_count; ++i)
        wings[i]->selected = false;
}

bool Model::move_selected(vec3 move_xyz, vec3 target_yz) {
    bool requires_reloft = false;

    for (int i = 0; i < objects_count; ++i) {
        Object *o = objects[i];
        if (o->selected)
            o->move(move_xyz);
    }

    for (int i = 0; i < wings_count; ++i) {
        Wing *w = wings[i];
        if (w->selected)
            wing_move_target_position(w, move_xyz.x, move_xyz.y, move_xyz.z);
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
