#include "modeling_model.h"
#include "modeling_object.h"
#include "modeling_wing.h"
#include <math.h>
#include <assert.h>
#include <string.h>


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
    assert(m->objects_count + m->wings_count < MAX_ELEMS);
    m->objects[m->objects_count++] = o;
}

void model_add_wing(Model *m, Wing *w) {
    assert(m->objects_count + m->wings_count < MAX_ELEMS);
    m->wings[m->wings_count++] = w;
}

void model_deselect_all(Model *m) {
    for (int i = 0; i < m->objects_count; ++i)
        m->objects[i]->selected = false;
    for (int i = 0; i < m->wings_count; ++i) {
        m->wings[i]->selected = false;
        m->wings[i]->selected = false;
    }
}

bool model_move_selected(Model *m, vec3 move_xyz, vec3 target_yz) {
    bool requires_reloft = false;

    for (int i = 0; i < m->objects_count; ++i) {
        Object *o = m->objects[i];
        if (o->selected)
            object_move(o, move_xyz);
    }

    for (int i = 0; i < m->wings_count; ++i) {
        Wing *w = m->wings[i];
        if (w->selected)
            wing_move_target_position(w, move_xyz.x, move_xyz.y, move_xyz.z);
    }

    return requires_reloft;
}

bool model_delete_selected(Model *m) {
    bool requires_reloft = false;

    /* remove selected objects */

    for (int i = 0; i < m->objects_count;) {
        Object *o = m->objects[i];

        if (o->selected) {
            --m->objects_count;
            for (int j = i; j < m->objects_count; ++j)
                m->objects[j] = m->objects[j + 1];
            requires_reloft = true;
        }
        else
            ++i;
    }

    return requires_reloft;
}

#ifndef NDEBUG

/* Serializes model before asserting. Label is used as dump file name. */
void _model_assert_func(Model *model, bool expr, const char *label) {
    if (!expr) {
        char path[512];
        strcpy_s(path, label);
        strcat_s(path, ".dump");
        model_serial_dump(model, path);
        assert(false);
    }
}

#endif
