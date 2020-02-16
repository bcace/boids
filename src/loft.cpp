#include "loft.h"
#include "mesh.h"
#include "model.h"
#include "fuselage.h"
#include "object.h"
#include "arena.h"
#include "math.h"
#include <math.h>
#include <string.h>


static Arena verts_arena(500000);
static Arena trias_arena(10000000);
static Arena quads_arena(10000000);

/* Circle used to roughly describe object cross-section for grouping objects into fuselages. */
struct _Circle {
    double x, y, r;
};

/* Approximation of the area in y-z plane shadowed by the object. */
static _Circle _object_circle(Object *o, bool is_clone) {
    _Circle c;
    double rx = ((double)o->max_y - (double)o->min_y) * 0.5;
    double ry = ((double)o->max_z - (double)o->min_z) * 0.5;
    c.x = o->min_y + rx;
    c.y = o->min_z + ry;
    c.r = max_d(rx, ry) + 0.05; /* a small margin is added to the circle */
    if (is_clone)
        c.x = -c.x;
    return c;
}

/* Returns true if circles representing objects in the y-z plane overlap. */
static bool _object_circles_overlap(Objref *a_ref, Objref *b_ref) {
    _Circle c_a = _object_circle(a_ref->object, a_ref->is_clone);
    _Circle c_b = _object_circle(b_ref->object, b_ref->is_clone);
    double dx = c_a.x - c_b.x;
    double dy = c_a.y - c_b.y;
    double dl = sqrt(dx * dx + dy * dy); // FAST_SQRT
    return dl < c_a.r + c_b.r;
}

/* Initializes an Objref instance. */
static void _init_objref(Objref *r, Object *o, bool is_clone) {
    r->object = o;
    r->is_clone = is_clone;
    r->fuselage_id = -1;
    r->t_conns_count = 0;
    r->n_conns_count = 0;
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

/* Main loft function. */
void loft_model(Arena *arena, Model *model) {

    if (model->objects.count == 0) /* if model has no objects we're done */
        return;

    arena->clear();

    /* clear old fuselages */

    static Fuselage fuselages[MAX_FUSELAGES];
    int fuselages_count = 0;

    for (int i = 0; i < MAX_FUSELAGES; ++i) {
        fuselages[i].objects_count = 0;
        fuselages[i].conns_count = 0;
    }

    /* create object references from model objects, including clones */

    Objref *o_refs = arena->alloc<Objref>(model->objects.count * 2);
    int o_refs_count = 0;

    for (int i = 0; i < model->objects.count; ++i) {
        Object *o = model->objects[i];

        break_assert(o_refs_count < MAX_FUSELAGE_OBJECTS);
        Objref *o_ref = o_refs + o_refs_count++;
        _init_objref(o_ref, o, false);

        /* cloning object if required, also make sure all clones follow the original object immediately */

        if (object_should_be_mirrored(o)) {
            break_assert(o_refs_count < MAX_FUSELAGE_OBJECTS);
            Objref *c_ref = o_refs + o_refs_count++;
            _init_objref(c_ref, o, true);
        }
    }

    /* let objrefs "infect" each other with fuselage ids if their area circles overlap */

    int next_available_fuselage_id = 0;
    for (int a_i = 0; a_i < o_refs_count; ++a_i) { /* assign fuselage ids to all objects */
        Objref *a_ref = o_refs + a_i;
        Object *a = a_ref->object;

        if (a_ref->fuselage_id == -1)
            a_ref->fuselage_id = next_available_fuselage_id++;

        for (int b_i = a_i + 1; b_i < o_refs_count; ++b_i) {
            Objref *b_ref = o_refs + b_i;
            Object *b = b_ref->object;

            if (!_object_circles_overlap(a_ref, b_ref)) /* if objects a and b do not overlap in y-z */
                continue;

            if (b_ref->fuselage_id == -1)
                b_ref->fuselage_id = a_ref->fuselage_id;
            else if (b_ref->fuselage_id != a_ref->fuselage_id) {
                for (int k = 0; k < o_refs_count; ++k) { /* switch all objects with the same fuselage id as object b to a */
                    Objref *c_ref = o_refs + k;
                    if (c_ref->fuselage_id == b_ref->fuselage_id)
                        c_ref->fuselage_id = a_ref->fuselage_id;
                }
            }
        }
    }

    /* create fuselages from fuselage ids */

    {
        static Fuselage *fuselages_map[MAX_ELEMENTS];
        for (int i = 0; i < MAX_ELEMENTS; ++i) // TODO: memset?
            fuselages_map[i] = 0;

        for (int i = 0; i < o_refs_count; ++i) { /* assign objects to fuselages, create fuselages if needed */
            Objref *o_ref = o_refs + i;
            break_assert(o_ref->fuselage_id != -1);

            Fuselage *fuselage = fuselages_map[o_ref->fuselage_id];
            if (fuselage == 0) {
                fuselage = fuselages + fuselages_count++;
                fuselages_map[o_ref->fuselage_id] = fuselage;
            }
            else
                fuselage = fuselages_map[o_ref->fuselage_id];

            fuselage->objects[fuselage->objects_count++] = *o_ref;
        }
    }

    /* create connections between objrefs in each fuselage */

    // TODO: explain how connection generation works

    struct _Conn1 {
        int t_i;
        int n_i;
    } *conns1 = arena->alloc<_Conn1>(MAX_FUSELAGE_OBJECTS * MAX_FUSELAGE_OBJECTS);

    struct _Conn2 {
        int t_i;
        int n_i;
        float grade;
    } *conns2 = arena->alloc<_Conn2>(MAX_FUSELAGE_OBJECTS * MAX_FUSELAGE_OBJECTS);

    struct _ObjrefInfo {
        OriginFlags t_conn_flags;
        OriginFlags n_conn_flags;
        OriginFlags non_clone_origin; /* origin flag where original object and all its clones have the same origin */
        OriginFlags t_non_clone_origins; /* non-clone-origins of all (potentially) connected objects */
        OriginFlags n_non_clone_origins; /* non-clone-origins of all (potentially) connected objects */
    } *infos = arena->alloc<_ObjrefInfo>(MAX_FUSELAGE_OBJECTS);

    for (int i = 0; i < fuselages_count; ++i) {
        Fuselage *fuselage = fuselages + i;

        memset(infos, 0, sizeof(_ObjrefInfo) * fuselage->objects_count);

        /* set objects' non-clone origins */

        OriginFlags last_non_clone_origin = ZERO_ORIGIN_FLAG; /* last non-clone is immediately followed by its clones */

        for (int o_i = 0; o_i < fuselage->objects_count; ++o_i) {
            Objref *o_ref = fuselage->objects + o_i;
            _ObjrefInfo *o_info = infos + o_i;

            if (o_ref->is_clone) {
                break_assert(last_non_clone_origin != ZERO_ORIGIN_FLAG);
                o_info->non_clone_origin = last_non_clone_origin;
            }
            else {
                last_non_clone_origin = ORIGIN_PART_TO_FLAG(fuselage->objects_count);
                o_info->non_clone_origin = last_non_clone_origin;
            }
        }

        /* determine all the possible connected pairs of objects (don't overlap along x) */

        int conns1_count = 0;

        for (int a_i = 0; a_i < fuselage->objects_count; ++a_i) {
            Objref *a_ref = fuselage->objects + a_i;
            Object *a = a_ref->object;
            _ObjrefInfo *a_info = infos + a_i;

            for (int b_i = a_i + 1; b_i < fuselage->objects_count; ++b_i) {
                Objref *b_ref = fuselage->objects + b_i;
                Object *b = b_ref->object;
                _ObjrefInfo *b_info = infos + b_i;

                if (a->max_x < b->min_x - 0.1) {        /* a is tail, b is nose */
                    if (_object_circles_overlap(a_ref, b_ref)) {
                        a_info->n_conn_flags |= ORIGIN_PART_TO_FLAG(b_i);
                        b_info->t_conn_flags |= ORIGIN_PART_TO_FLAG(a_i);
                        _Conn1 *c1 = conns1 + conns1_count++;
                        c1->t_i = a_i;
                        c1->n_i = b_i;
                    }
                }
                else if (a->min_x > b->max_x + 0.1) {   /* a is nose, b is tail */
                    if (_object_circles_overlap(a_ref, b_ref)) {
                        a_info->t_conn_flags |= ORIGIN_PART_TO_FLAG(b_i);
                        b_info->n_conn_flags |= ORIGIN_PART_TO_FLAG(a_i);
                        _Conn1 *c1 = conns1 + conns1_count++;
                        c1->t_i = b_i;
                        c1->n_i = a_i;
                    }
                }
            }
        }

        /* create connection candidates by filtering through possible ones */

        int conns2_count = 0;

        for (int j = 0; j < conns1_count; ++j) {
            _Conn1 *c1 = conns1 + j;
            _ObjrefInfo *t_info = infos + c1->t_i;
            _ObjrefInfo *n_info = infos + c1->n_i;

            /* skip connection candidate if objects it connects can be connected through other objects in between */

            if ((t_info->n_conn_flags & n_info->t_conn_flags) != 0) /* tail object is nosewise connected to some objects nose object is tailwise connected to */
                continue;

            Object *t_obj = fuselage->objects[c1->t_i].object;
            Object *n_obj = fuselage->objects[c1->n_i].object;

            float dx = n_obj->min_x - t_obj->max_x;
            // TODO: see if this should actually be appropriate skin former centroids instead of just object positions
            float dy = n_obj->p.y - t_obj->p.y;
            float dz = n_obj->p.z - t_obj->p.z;
            // TODO: check if I can remove this sqrtf altogether
            float grade = dx / sqrtf(dy * dy + dz * dz); // FAST_SQRT

            int insert_i = 0;
            for (; insert_i < conns2_count; ++insert_i)
                if (grade > conns2[insert_i].grade)
                    break;
            for (int m = conns2_count; m > insert_i; --m)
                conns2[m] = conns2[m - 1];

            _Conn2 *c2 = conns2 + insert_i;
            c2->t_i = c1->t_i;
            c2->n_i = c1->n_i;
            t_info->n_non_clone_origins |= n_info->non_clone_origin;
            n_info->t_non_clone_origins |= t_info->non_clone_origin;
            c2->grade = grade;
            ++conns2_count;
        }

        /* make actual conns from conn candidates */

        for (int j = 0; j < conns2_count; ++j) {
            _Conn2 *c2 = conns2 + j;
            _ObjrefInfo *t_info = infos + c2->t_i;
            _ObjrefInfo *n_info = infos + c2->n_i;

            /* only add the connection if both endpoints don't already have something connected */

            if ((t_info->n_non_clone_origins & ~n_info->non_clone_origin) == ZERO_ORIGIN_FLAG ||
                (n_info->t_non_clone_origins & ~t_info->non_clone_origin) == ZERO_ORIGIN_FLAG) {

                Conn *c = fuselage->conns + fuselage->conns_count++;
                c->tail_o = fuselage->objects + c2->t_i;
                c->nose_o = fuselage->objects + c2->n_i;
                ++c->tail_o->n_conns_count;
                ++c->nose_o->t_conns_count;
            }
        }
    }

    /* generate skin panels */

    verts_arena.clear();
    model->skin_verts = verts_arena.rest<vec3>();
    model->skin_verts_count = 0;
    mesh_init(model);

    for (int i = 0; i < fuselages_count; ++i) {
        arena->clear();
        fuselage_loft(arena, &verts_arena, model, fuselages + i);
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
