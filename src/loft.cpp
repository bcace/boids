#include "loft.h"
#include "mesh.h"
#include "model.h"
#include "fuselage.h"
#include "object.h"
#include "arena.h"
#include "math.h"
#include <math.h>


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
        c.x -= o->p.y * 2.0;
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
static void _init_objref(Objref *o_ref, Object *o, bool is_clone) {
    o_ref->object = o;
    o_ref->is_clone = is_clone;
    o_ref->fuselage_id = -1;
    o_ref->t_conns_count = 0;
    o_ref->n_conns_count = 0;
    o_ref->t_conn_flags = ZERO_ORIGIN_FLAG;
    o_ref->n_conn_flags = ZERO_ORIGIN_FLAG;
    o_ref->x = (double)o->p.x;
    o_ref->z = (double)o->p.z;
    if (is_clone) {
        o_ref->y = -(double)o->p.y;
        shape_mirror_former(&o->tail_skin_former, &o_ref->t_skin_former, o_ref->x, o_ref->y, o_ref->z, STRUCTURAL_MARGIN);
        shape_mirror_former(&o->nose_skin_former, &o_ref->n_skin_former, o_ref->x, o_ref->y, o_ref->z, STRUCTURAL_MARGIN);
    }
    else {
        o_ref->y = (double)o->p.y;
        shape_copy_former(&o->tail_skin_former, &o_ref->t_skin_former, o_ref->x, o_ref->y, o_ref->z, STRUCTURAL_MARGIN);
        shape_copy_former(&o->nose_skin_former, &o_ref->n_skin_former, o_ref->x, o_ref->y, o_ref->z, STRUCTURAL_MARGIN);
    }
}

/* Main loft function. */
void loft_model(Arena *arena, Model *model) {
    static Fuselage fuselages[MAX_FUSELAGES];
    int fuselages_count = 0;

    arena->clear();

    for (int i = 0; i < MAX_FUSELAGES; ++i) { /* clear old fuselages */
        fuselages[i].objects_count = 0;
        fuselages[i].conns_count = 0;
    }

    if (model->objects.count == 0) /* if model has no objects we're done */
        return;

    /* create object references from model objects, including clones */

    Objref *o_refs = arena->alloc<Objref>(model->objects.count * 2);
    int o_refs_count = 0;

    for (int i = 0; i < model->objects.count; ++i) {
        Object *o = model->objects[i];
        _init_objref(o_refs + o_refs_count++, o, false);
        if (object_should_be_mirrored(o))
            _init_objref(o_refs + o_refs_count++, o, true);
    }

    // TODO: describe how fuselage id propagation works

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

        for (int i = 0; i < o_refs_count; ++i) {
            Objref *o_ref = o_refs + i;
            break_assert(o_ref->fuselage_id != -1);

            Fuselage *fuselage = fuselages_map[o_ref->fuselage_id];
            if (fuselage == 0) {
                fuselage = fuselages + fuselages_count++;
                fuselages_map[o_ref->fuselage_id] = fuselage;
            }
            else
                fuselage = fuselages_map[o_ref->fuselage_id];

            fuselage->objects[fuselage->objects_count++] = o_refs[i];
        }
    }

    // TODO: explain how connection generation works

    typedef struct {
        Objref *tail_o;
        Objref *nose_o;
        float grade;
    } _ConnCandidate;

    _ConnCandidate *candidates = arena->alloc<_ConnCandidate>(MAX_FUSELAGE_OBJECTS * MAX_FUSELAGE_OBJECTS);

    for (int i = 0; i < fuselages_count; ++i) {
        Fuselage *fuselage = fuselages + i;

        for (int a_i = 0; a_i < fuselage->objects_count; ++a_i) { /* determine all the possible connected pairs of objects */
            Objref *a_ref = fuselage->objects + a_i;
            Object *a = a_ref->object;

            for (int b_i = a_i + 1; b_i < fuselage->objects_count; ++b_i) {
                Objref *b_ref = fuselage->objects + b_i;
                Object *b = b_ref->object;

                if (a->max_x < b->min_x - 0.1) {        /* a is tail, b is nose */
                    if (_object_circles_overlap(a_ref, b_ref)) {
                        a_ref->n_conn_flags |= ORIGIN_PART_TO_FLAG(b_i);
                        b_ref->t_conn_flags |= ORIGIN_PART_TO_FLAG(a_i);
                    }
                }
                else if (a->min_x > b->max_x + 0.1) {   /* a is nose, b is tail */
                    if (_object_circles_overlap(a_ref, b_ref)) {
                        a_ref->t_conn_flags |= ORIGIN_PART_TO_FLAG(b_i);
                        b_ref->n_conn_flags |= ORIGIN_PART_TO_FLAG(a_i);
                    }
                }
            }
        }

        int conns_count = 0;
        for (int a_i = 0; a_i < fuselage->objects_count; ++a_i) { /* create connection candidates by filtering through possible ones */
            Objref *a_ref = fuselage->objects + a_i;
            Object *a = a_ref->object;

            for (int b_i = 0; b_i < fuselage->objects_count; ++b_i) {
                if ((a_ref->t_conn_flags & ORIGIN_PART_TO_FLAG(b_i)) == 0) /* object b is not candidate for connection with object a */
                    continue;

                Objref *b_ref = fuselage->objects + b_i;
                Object *b = b_ref->object;

                if ((b_ref->n_conn_flags & a_ref->t_conn_flags) != 0) /* object b is nosewise connected to some objects a is tailwise connected */
                    continue;

                float dx = a->min_x - b->max_x;
                // TODO: see if this should actually be appropriate skin former centroids instead of just object positions
                float dy = a->p.y - b->p.y;
                float dz = a->p.z - b->p.z;
                // TODO: check if I can remove this sqrtf altogether
                float grade = dx / sqrtf(dy * dy + dz * dz); // FAST_SQRT

                int insert_i = 0;
                for (; insert_i < conns_count; ++insert_i)
                    if (grade > candidates[insert_i].grade)
                        break;
                for (int m = conns_count; m > insert_i; --m)
                    candidates[m] = candidates[m - 1];

                _ConnCandidate *cc = candidates + insert_i;
                cc->tail_o = b_ref;
                cc->nose_o = a_ref;
                cc->grade = grade;
                ++conns_count;
            }
        }

        for (int j = 0; j < conns_count; ++j) { /* make actual conns from conn candidates */
            _ConnCandidate *cc = candidates + j;

            /* only add the connection if both endpoints don't already have something connected */
            if (cc->tail_o->n_conns_count == 0 || cc->nose_o->t_conns_count == 0) {
                Conn *c = fuselage->conns + fuselage->conns_count++;
                c->tail_o = cc->tail_o;
                c->nose_o = cc->nose_o;
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

    for (int i = 0; i < fuselages_count; ++i)
        fuselage_loft(arena, &verts_arena, model, fuselages + i);

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
