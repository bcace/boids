#include "modeling_model.h"
#include "modeling_mesh.h"
#include "modeling_fuselage.h"
#include "modeling_object.h"
#include "modeling_wing.h"
#include "util_group.h"
#include "memory_arena.h"
#include <math.h>
#include <string.h>


static Arena verts_arena(500000);
static Arena trias_arena(10000000);
static Arena quads_arena(10000000);

/* Initializes an Objref instance. */
static void _init_oref(Oref *r, Object *o, int index, bool is_clone) {
    memset(r, 0, sizeof(Oref));
    r->object = o;
    r->is_clone = is_clone;
    r->non_clone_origin = flags_make(index);
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

static void _init_wref(Wref *wref, Wing *w, int index, bool is_clone) {
    wref->wing = w;
    wref->is_clone = is_clone;
}

/* Main loft function. */
void model_loft(Arena *arena, Model *model) {
    if (model->objects_count == 0) /* if model has no objects we're done */
        return;

    arena->clear();

    static Fuselage fuselages[MAX_FUSELAGES];
    int fuselages_count = 0;
    int available_ref_index = 0;

    /* create object references from model objects, including clones */

    Oref *orefs = arena->alloc<Oref>(model->objects_count * 2);
    int orefs_count = 0;
    for (int i = 0; i < model->objects_count; ++i) {
        Object *o = model->objects[i];
        Oref *oref = orefs + orefs_count++;
        _init_oref(oref, o, available_ref_index, false);
        if (object_should_be_mirrored(o)) {
            Oref *c_ref = orefs + orefs_count++;
            _init_oref(c_ref, o, available_ref_index, true);
        }
        ++available_ref_index;
    }

    /* create wing references */

    Wref *wrefs = arena->alloc<Wref>(model->wings_count * 2);
    int wrefs_count = 0;
    for (int i = 0; i < model->wings_count; ++i) {
        Wing *w = model->wings[i];
        Wref *wref = wrefs + wrefs_count++;
        _init_wref(wref, w, available_ref_index, false);
        if (wing_should_be_mirrored(w)) {
            Wref *_wref = wrefs + wrefs_count++;
            _init_wref(_wref, w, available_ref_index, true);
        }
        ++available_ref_index;
    }

    /* group object references into fuselages */

    static GroupMaker maker;
    group_objects(sizeof(Oref), orefs, orefs_count, (GROUP_FUNC)fuselage_objects_overlap, &maker);
    for (int i = 0; i < maker.count; ++i) {
        Group *g = maker.groups + i;
        Fuselage *f = fuselages + fuselages_count++;
        f->orefs_count = g->count;
        f->wrefs_count = 0;
        f->conns_count = 0;
        for (int j = 0; j < g->count; ++j)
            f->orefs[j] = orefs[g->obj_indices[j]];
    }

    /* add wings to fuselages */

    for (int i = 0; i < wrefs_count; ++i) {
        Wref *wref = wrefs + i;
        for (int j = 0; j < fuselages_count; ++j) {
            Fuselage *f = fuselages + j;
            for (int k = 0; k < f->orefs_count; ++k) {
                Oref *oref = f->orefs + k;
                if (fuselage_object_and_wing_overlap(oref, wref)) {
                    f->wrefs[f->wrefs_count++] = *wref;
                    goto FUSELAGE_FOR_WING_FOUND;
                }
            }
        }
        FUSELAGE_FOR_WING_FOUND:;
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
