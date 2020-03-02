#include "model.h"
#include "mesh.h"
#include "fuselage.h"
#include "object.h"
#include "group.h"
#include "arena.h"
#include <math.h>
#include <string.h>


static Arena verts_arena(500000);
static Arena trias_arena(10000000);
static Arena quads_arena(10000000);

/* Initializes an Objref instance. */
static void _init_objref(Objref *r, Object *o, int index, bool is_clone) {
    memset(r, 0, sizeof(Objref));
    r->object = o;
    r->is_clone = is_clone;
    r->non_clone_origin = origin_index_to_flag(index);
    r->x = (double)o->p.x;
    r->z = (double)o->p.z;
    if (r->is_clone) {
        r->y = -(double)o->p.y;
        shape_mirror_former(&o->tail_skin_former, &r->t_skin_former, r->x, r->y, r->z, STRUCTURAL_MARGIN);
        shape_mirror_former(&o->nose_skin_former, &r->n_skin_former, r->x, r->y, r->z, STRUCTURAL_MARGIN);
    }
    else {
        r->y = (double)o->p.y;
        shape_copy_former(&o->tail_skin_former, &r->t_skin_former, r->x, r->y, r->z, STRUCTURAL_MARGIN);
        shape_copy_former(&o->nose_skin_former, &r->n_skin_former, r->x, r->y, r->z, STRUCTURAL_MARGIN);
    }
}

bool _objects_overlap(Objref *a, Objref *b) {
    return object_overlap_in_yz(a->object, a->is_clone, b->object, b->is_clone);
}

/* Main loft function. */
void loft_model(Arena *arena, Model *model) {

    if (model->objects_count == 0) /* if model has no objects we're done */
        return;

    arena->clear();

    static Fuselage fuselages[MAX_FUSELAGES];
    int fuselages_count = 0;

    /* create object references from model objects, including clones */

    Objref *o_refs = arena->alloc<Objref>(model->objects_count * 2);
    int o_refs_count = 0;

    break_assert(model->objects_count <= MAX_FUSELAGE_OBJECTS); // TODO: remove this once max number of objects in the model is the same as that of fuselages

    for (int i = 0; i < model->objects_count; ++i) {
        Object *o = model->objects[i];

        Objref *o_ref = o_refs + o_refs_count++;
        _init_objref(o_ref, o, i, false);

        if (object_should_be_mirrored(o)) {
            Objref *c_ref = o_refs + o_refs_count++;
            _init_objref(c_ref, o, i, true);
        }
    }

    /* group object references into fuselages */

    static GroupMaker maker;
    group_objects(sizeof(Objref), o_refs, o_refs_count, (GROUP_FUNC)_objects_overlap, &maker);

    for (int i = 0; i < maker.count; ++i) {
        Group *g = maker.groups + i;

        Fuselage *f = fuselages + fuselages_count++;
        f->objects_count = g->count;
        f->conns_count = 0;

        for (int j = 0; j < g->count; ++j)
            f->objects[j] = o_refs[g->obj_indices[j]];
    }

    /* generate skin panels */

    verts_arena.clear();
    model->skin_verts = verts_arena.rest<vec3>();
    model->skin_verts_count = 0;
    mesh_init(model);

    for (int i = 0; i < fuselages_count; ++i) {
        Fuselage *f = fuselages + i;
        arena->clear();
        fuselage_update_conns(arena, f);
        fuselage_update_longitudinal_tangents(f);
        arena->clear();
        fuselage_loft(arena, &verts_arena, model, f);
    }

    /* make triangles and quads for drawing from generated panels */

    trias_arena.clear();
    model->skin_trias = trias_arena.rest<int>();
    model->skin_trias_count = 0;

    quads_arena.clear();
    model->skin_quads = quads_arena.rest<int>();
    model->skin_quads_count = 0;

    for (int i = 0; i < model->panels_count; ++i) {
        Panel *p = model->panels + i;
        if (p->v4 == -1) {  /* triangle */
            int *idx = trias_arena.alloc<int>(3); // TODO: think about pre-allocating this
            *idx++ = p->v1;
            *idx++ = p->v2;
            *idx++ = p->v3;
            ++model->skin_trias_count;
        }
        else {              /* quad */
            int *idx = quads_arena.alloc<int>(4); // TODO: think about pre-allocating this
            *idx++ = p->v1;
            *idx++ = p->v2;
            *idx++ = p->v3;
            *idx++ = p->v4;
            ++model->skin_quads_count;
        }
    }
}
