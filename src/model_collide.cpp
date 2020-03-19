#include "model.h"
#include "object.h"
#include "ochre.h"
#include "collision.h"
#include "arena.h"
#include "config.h"


static OcState *state;
static OcNodeGroup *objects_group;

/* Action callback that prepares an object for interaction. */
static void _object_preparation(void *agent, void *exec_context) {
    CollContext *c = (CollContext *)exec_context;
    Arena *arena = c->arena;

    Object *o = (Object *)agent;
    o->prisms = arena->rest<CollPrism>();
    o->prisms_count = 0;
    o->bounds.reset();

    float x = o->p.x;
    float y = o->p.y;
    float z = o->p.z;

    for (int i = 0; i < o->formers_count; ++i) { /* a prism for each object former */
        Former *f = o->formers + i;

        CollPrism *prism = arena->alloc<CollPrism>();
        o->prisms_count++;

        coll_get_prism(&f->shape, prism, y, z, STRUCTURAL_MARGIN * 0.5);
        prism->x = f->x + x;

        if (i == 0) /* first former */
            prism->min_x = f->x + x;
        else
            prism->min_x = (f->x + o->formers[i - 1].x) * 0.5f + x;

        if (i == o->formers_count - 1) /* last former */
            prism->max_x = f->x + x;
        else
            prism->max_x = (f->x + o->formers[i + 1].x) * 0.5f + x;

        for (int j = 0; j < COLL_VERTS_PER_PRISM; ++j) /* update bounds */
            o->bounds.include(prism->verts[j].x, prism->verts[j].y);
    }

    o->coll_min_x = (float)o->prisms[0].x;
    o->coll_max_x = (float)o->prisms[o->prisms_count - 1].x;
    o->min_x = x + o->tail_skin_former.x;
    o->max_x = x + o->nose_skin_former.x;
}

/* Interaction callback that describes interaction between objects. */
static void _object_interaction(void *agent1, void *agent2, void *exec_context) {
    Object *o1 = (Object *)agent1;
    Object *o2 = (Object *)agent2;

    /* broad phase */

    if (!o1->bounds.intersects(o2->bounds)) /* no overlap in y-z plane */
        return;

    if (o1->coll_min_x > o2->coll_max_x || o1->coll_max_x < o2->coll_min_x) /* no overlap along x */
        return;

    /* narrow phase */

    double f_sum[3];
    bool interacted = false;

    f_sum[0] = f_sum[1] = f_sum[2] = 0.0;

    for (int i1 = 0; i1 < o1->prisms_count; ++i1) {
        CollPrism *p1 = o1->prisms + i1;

        for (int i2 = 0; i2 < o2->prisms_count; ++i2) {
            CollPrism *p2 = o2->prisms + i2;

            double f[3];
            if (coll_interact_prisms(p1, p2, f)) {
                f_sum[0] += f[0];
                f_sum[1] += f[1];
                f_sum[2] += f[2];
                interacted = true;
            }
        }
    }

    if (interacted) {
        o1->f.x -= (float)f_sum[0];
        o1->f.y -= (float)f_sum[1];
        o1->f.z -= (float)f_sum[2];
        o2->f.x += (float)f_sum[0];
        o2->f.y += (float)f_sum[1];
        o2->f.z += (float)f_sum[2];
    }
}

/* Interaction callback that describes interaction between objects and symmetry plane. */
static void _object_plane_interaction(void *agent, void *exec_context) {
    Object *o = (Object *)agent;
    if (o->p.y < 0.0f)
        o->f.y -= o->p.y;
    else if (object_should_be_centered(o))
        o->f.y -= o->p.y * 1.0f; // TODO: experiment and document what this factor is
}

/* Action callback for objects to finalize stuff. */
static void _object_action(void *agent, void *exec_context) {
    CollContext *c = (CollContext *)exec_context;
    Object *o = (Object *)agent;
    if (o->selected && c->dragging)
        o->f -= o->p - o->drag_p;
}

/* Initialize ochre state for model elements collision. */
void model_init_ochre_state() {
    state = ochre_add_state();
    objects_group = ochre_add_node_group(state, OFFSETOF(Object, f), OFFSETOF(Object, p), OC_LAYOUT_F32_3);
    ochre_add_node_action(state, objects_group, _object_preparation, 0);
    ochre_add_node_interaction(state, objects_group, objects_group, _object_interaction, 1);
    ochre_add_node_action(state, objects_group, _object_plane_interaction, 1);
    ochre_add_node_action(state, objects_group, _object_action, 2);
}

/* Main model elements collision procedure. Returns true if some elements moved
which would require relofting. */
bool Model::collide(Arena *arena, bool dragging) {
    CollContext ctx;
    ctx.arena = arena;
    ctx.dragging = dragging;

    arena->clear();
    ochre_set_exec_context(state, &ctx);

    /* to collider */
    ochre_clear_data(state);
    for (int i = 0; i < objects_count; ++i) /* object nodes */
        ochre_add_node(objects_group, objects[i]);

    /* collide */
    if (!ochre_run(state, 10))
        return false;

    /* postprocess */
    for (int i = 0; i < objects_count; ++i) {
        Object *o = objects[i];
        if (object_should_be_centered(o))
            o->p.y = 0.0f;
        object_update_extents(o);
    }

    return true;
}
