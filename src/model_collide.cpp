#include "model.h"
#include "object.h"
#include "ochre.h"
#include "arena.h"


static OcState *state;
static OcNodeGroup *objects_group;

void model_init_ochre_state() {
    state = ochre_add_state();

    objects_group = ochre_add_node_group(state, OFFSETOF(Object, f), OFFSETOF(Object, p), OC_LAYOUT_F32_3);

    ochre_add_node_action(state, objects_group, object_preparation, 0);
    ochre_add_node_interaction(state, objects_group, objects_group, object_interaction, 1);
    ochre_add_node_action(state, objects_group, object_plane_interaction, 1);
    ochre_add_node_action(state, objects_group, object_action, 2);
}

bool Model::collide(Arena *arena, bool dragging) {

    arena->clear();

    ochre_set_exec_context(state, arena);

    // to collider
    {

        ochre_clear_data(state);

        // object nodes
        for (int i = 0; i < objects_count; ++i) {
            Object *o = objects[i];
            o->dragging = o->selected && dragging;
            ochre_add_node(objects_group, o);
        }
    }

    // collide
    if (!ochre_run(state, 10))
        return false;

    // postprocess
    for (int i = 0; i < objects_count; ++i) {
        Object *o = objects[i];
        if (object_should_be_centered(o))
            o->p.y = 0.0f;
        object_update_extents(o);
    }

    return true;
}
