#include "modeling_fuselage.h"
#include "modeling_object.h"
#include "memory_arena.h"
#include "math_dvec.h"
#include <string.h>
#include <math.h>


/* Intermediate connection representation used while building connections. */
struct _Conn {
    int t_i;
    int n_i;
    float grade;
};

void fuselage_update_conns(Arena *arena, Fuselage *fuselage) {

    /* determine all the possible connected pairs of objects (don't overlap along x) */

    _Conn *conns1 = arena->alloc<_Conn>(MAX_ELEM_REFS * MAX_ELEM_REFS);
    int conns1_count = 0;

    for (int a_i = 0; a_i < fuselage->orefs_count; ++a_i) {
        Oref *a_ref = fuselage->orefs + a_i;
        Object *a = a_ref->object;

        for (int b_i = a_i + 1; b_i < fuselage->orefs_count; ++b_i) {
            Oref *b_ref = fuselage->orefs + b_i;
            Object *b = b_ref->object;

            if (a->max_x < b->min_x - 0.1) {        /* a is tail, b is nose */
                if (fuselage_objects_overlap(a_ref, b_ref)) {
                    flags_add(&a_ref->n.conn_flags, b_i);
                    flags_add(&b_ref->t.conn_flags, a_i);
                    _Conn *c1 = conns1 + conns1_count++;
                    c1->t_i = a_i;
                    c1->n_i = b_i;
                }
            }
            else if (a->min_x > b->max_x + 0.1) {   /* a is nose, b is tail */
                if (fuselage_objects_overlap(a_ref, b_ref)) {
                    flags_add(&a_ref->t.conn_flags, b_i);
                    flags_add(&b_ref->n.conn_flags, a_i);
                    _Conn *c1 = conns1 + conns1_count++;
                    c1->t_i = b_i;
                    c1->n_i = a_i;
                }
            }
        }
    }

    /* create connection candidates by filtering through possible ones */

    _Conn *conns2 = arena->alloc<_Conn>(MAX_ELEM_REFS * MAX_ELEM_REFS);
    int conns2_count = 0;

    for (int j = 0; j < conns1_count; ++j) {
        _Conn *c1 = conns1 + j;
        Oref *t_ref = fuselage->orefs + c1->t_i;
        Oref *n_ref = fuselage->orefs + c1->n_i;

        /* skip connection candidate if objects it connects can be connected through other objects in between */

        if (flags_and(&t_ref->n.conn_flags, &n_ref->t.conn_flags)) /* tail object is nosewise connected to some objects nose object is tailwise connected to */
            continue;

        Object *t_obj = t_ref->object;
        Object *n_obj = n_ref->object;

        float dx = n_obj->min_x - t_obj->max_x;
        // TODO: see if this should actually be appropriate skin former centroids instead of just object positions
        float dy = n_obj->p.y - t_obj->p.y;
        float dz = n_obj->p.z - t_obj->p.z;
        float grade = dx / sqrtf(dy * dy + dz * dz); // FAST_SQRT

        int insert_i = 0;
        for (; insert_i < conns2_count; ++insert_i)
            if (grade > conns2[insert_i].grade)
                break;
        for (int m = conns2_count; m > insert_i; --m)
            conns2[m] = conns2[m - 1];

        _Conn *c2 = conns2 + insert_i;
        c2->t_i = c1->t_i;
        c2->n_i = c1->n_i;
        c2->grade = grade;
        ++conns2_count;
    }

    /* make actual conns from conn candidates */

    for (int j = 0; j < conns2_count; ++j) {
        _Conn *c2 = conns2 + j;
        Oref *t_ref = fuselage->orefs + c2->t_i;
        Oref *n_ref = fuselage->orefs + c2->n_i;

        /* skip connection if both endpoints already have something connected */

        if (flags_has_anything_other_than(&t_ref->n.non_clone_origins, &n_ref->non_clone_origin) &&
            flags_has_anything_other_than(&n_ref->t.non_clone_origins, &t_ref->non_clone_origin))
            continue;

        flags_add_flags(&t_ref->n.non_clone_origins, &n_ref->non_clone_origin);
        flags_add_flags(&n_ref->t.non_clone_origins, &t_ref->non_clone_origin);

        Conn *c = fuselage->conns + fuselage->conns_count++;
        c->tail_o = fuselage->orefs + c2->t_i;
        c->nose_o = fuselage->orefs + c2->n_i;
        ++c->tail_o->n_conns_count;
        ++c->nose_o->t_conns_count;
    }
}

/* Longitudinal tangents are used to interpolate curve control points along pipes. */
tvec _calculate_longitudinal_tangent_direction(double px, double py, double pz,
                                               double tx, double ty, double tz,
                                               double nx, double ny, double nz) {
    tvec td = tvec_init(tx - px, ty - py, tz - pz);
    td = tvec_norm(td);

    tvec nd = tvec_init(nx - px, ny - py, nz - pz);
    nd = tvec_norm(nd);

    double t_angle = acos(-td.x);
    double n_angle = acos( nd.x);
    double sum_angle = t_angle + n_angle;

    if (sum_angle == 0.0)
        return tvec_zero();

    nd = tvec_scale(nd, t_angle / sum_angle);
    td = tvec_scale(td, n_angle / sum_angle);
    tvec d = tvec_sub(nd, td);

    return tvec_norm(d);
}

void fuselage_update_longitudinal_tangents(Fuselage *fuselage) {

    /* zero longitudinal tangents */

    for (int i = 0; i < fuselage->orefs_count; ++i) {
        Oref *o_ref = fuselage->orefs + i;
        for (int j = 0; j < SHAPE_CURVES; ++j) {
            o_ref->t_tangents[j] = tvec_zero();
            o_ref->n_tangents[j] = tvec_zero();
        }
    }

    /* calculate conn tangents */

    for (int i = 0; i < fuselage->conns_count; ++i) {
        Conn *conn = fuselage->conns + i;
        Oref *t_ref = conn->tail_o;
        Oref *n_ref = conn->nose_o;

        Former *f1 = &t_ref->n_skin_former;
        Former *f2 = &t_ref->t_skin_former;
        Former *f3 = &n_ref->t_skin_former;
        Former *f4 = &n_ref->n_skin_former;
        tvec *t_tangents = t_ref->n_tangents;
        tvec *n_tangents = n_ref->t_tangents;

        /* tail */

        for (int j = 0; j < SHAPE_CURVES; ++j) {
            tvec d = _calculate_longitudinal_tangent_direction(
                f1->x, f1->shape.curves[j].x, f1->shape.curves[j].y,
                f2->x, f2->shape.curves[j].x, f2->shape.curves[j].y,
                f3->x, f3->shape.curves[j].x, f3->shape.curves[j].y
            );
            t_tangents[j] = tvec_add(t_tangents[j], d);
        }

        /* nose */

        for (int j = 0; j < SHAPE_CURVES; ++j) {
            tvec d = _calculate_longitudinal_tangent_direction(
                f3->x, f3->shape.curves[j].x, f3->shape.curves[j].y,
                f1->x, f1->shape.curves[j].x, f1->shape.curves[j].y,
                f4->x, f4->shape.curves[j].x, f4->shape.curves[j].y
            );
            n_tangents[j] = tvec_add(n_tangents[j], d);
        }
    }

    /* normalize conn tangents and calculate opening tangents */

    for (int i = 0; i < fuselage->orefs_count; ++i) {
        Oref *o_ref = fuselage->orefs + i;
        Object *o = o_ref->object;
        Former *tail_f = &o_ref->t_skin_former;
        Former *nose_f = &o_ref->n_skin_former;

        /* tail */

        if (o_ref->t_conns_count == 0) { /* opening tangents */
            for (int j = 0; j < SHAPE_CURVES; ++j) {
                o_ref->t_tangents[j] = _calculate_longitudinal_tangent_direction(
                    tail_f->x, tail_f->shape.curves[j].x, tail_f->shape.curves[j].y,
                    tail_f->x - o->tail_endp_dx, o_ref->y, o_ref->z,
                    nose_f->x, nose_f->shape.curves[j].x, nose_f->shape.curves[j].y
                );
            }
        }
        else { /* conn tangents */
            for (int j = 0; j < SHAPE_CURVES; ++j)
                o_ref->t_tangents[j] = tvec_norm(o_ref->t_tangents[j]);
        }

        /* nose */

        if (o_ref->n_conns_count == 0) { /* opening tangents */
            for (int j = 0; j < SHAPE_CURVES; ++j) {
                o_ref->n_tangents[j] = _calculate_longitudinal_tangent_direction(
                    nose_f->x, nose_f->shape.curves[j].x, nose_f->shape.curves[j].y,
                    tail_f->x, tail_f->shape.curves[j].x, tail_f->shape.curves[j].y,
                    nose_f->x + o->nose_endp_dx, o_ref->y, o_ref->z
                );
            }
        }
        else { /* conn tangents */
            for (int j = 0; j < SHAPE_CURVES; ++j)
                o_ref->n_tangents[j] = tvec_norm(o_ref->n_tangents[j]);
        }
    }
}
