#include "object.h"
#include "collision.h"
#include "ochre.h"
#include "arena.h"
#include "interp.h"
#include "config.h"


void Object::prepare_for_collision(Arena *arena) {
    prisms_count = 0;
    prisms = 0;

    bounds.reset();

    for (int i = 0; i < formers_count; ++i) { /* section prisms */
        Former &form = formers[i];

        CollPrism *prism = arena->alloc<CollPrism>();
        if (prisms == 0)
            prisms = prism;
        prisms_count++;
        prism->x = form.x + p.x;

        coll_get_prism(&form.shape, prism, p.y, p.z, STRUCTURAL_MARGIN * 0.5);

        if (i == 0)
            prism->min_x = form.x + p.x; /* TODO: elongate this for e.g. engines */
        else
            prism->min_x = (form.x + formers[i - 1].x) * 0.5f + p.x;

        if (i == formers_count - 1)
            prism->max_x = form.x + p.x; /* TODO: elongate this for e.g. intakes */
        else
            prism->max_x = (form.x + formers[i + 1].x) * 0.5f + p.x;

        for (int j = 0; j < COLL_VERTS_PER_PRISM; ++j) /* update bounds */
            bounds.include(prism->verts[j].x, prism->verts[j].y);
    }

    coll_min_x = prisms[0].x; /* TODO: elongate this for e.g. engines */
    coll_max_x = prisms[prisms_count - 1].x; /* TODO: elongate this for e.g. intakes */
    min_x = p.x + tail_skin_former.x;
    max_x = p.x + nose_skin_former.x;
}

void object_preparation(void *agent, void *exec_context) {
    ((Object *)agent)->prepare_for_collision((Arena *)exec_context);
}

void object_interaction(void *agent1, void *agent2, void *exec_context) {
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
        o1->f.x -= f_sum[0];
        o1->f.y -= f_sum[1];
        o1->f.z -= f_sum[2];
        o2->f.x += f_sum[0];
        o2->f.y += f_sum[1];
        o2->f.z += f_sum[2];
    }
}

void object_plane_interaction(void *agent, void *exec_context) {
    Object *o = (Object *)agent;

    if (o->p.y < 0.0f)
        o->f.y -= o->p.y;
    else if (object_should_be_centered(o))
        o->f.y -= o->p.y * 1.0f; // TODO: experiment and document what this factor is
}

void object_action(void *agent, void *exec_context) {
    Object *o = (Object *)agent;
    if (o->dragging)
        o->f -= o->p - o->drag_p;
}
