#include "fuselage.h"
#include "wing.h"
#include "mesh.h"
#include "airfoil.h"
#include "periodic.h"
#include "dvec.h"
#include "arena.h"
#include "math.h"
#include <math.h>


static FuselageIsec _envelope_and_line_intersection(TraceEnv *env, dvec p1, dvec p2, double x) {
    FuselageIsec p;
    p.i = -1;
    p.t = 0.0;

    const int _MAX_TRIES = 100;
    const double _RETRY_STEP = 1.0e-10;
    const double _RETRY_MARGIN = 1.0e-15;
    const double _ONE_PLUS_RETRY_MARGIN = 1.0 + _RETRY_MARGIN;
    const double _ONE_MINUS_RETRY_MARGIN = 1.0 - _RETRY_MARGIN;
    const double _PARALLEL_MARGIN = 1.0e-10;

    /* line normal */
    double nx = p1.y - p2.y;
    double ny = p2.x - p1.x;
    double nl = sqrt(nx * nx + ny * ny);
    nx /= nl;
    ny /= nl;

    for (int try_i = 0; try_i < _MAX_TRIES; ++try_i) {

        /* if retrying move line by retry step in the direction of its normal */
        if (try_i) {
            double dx = nx * _RETRY_STEP * try_i;
            double dy = ny * _RETRY_STEP * try_i;
            p1.x += dx;
            p1.y += dy;
            p2.x += dx;
            p2.y += dy;
        }

        for (int i1 = 0; i1 < env->count; ++i1) {
            int i2 = period_incr(i1, env->count);
            EnvPoint *e1 = env->points + i1;
            EnvPoint *e2 = env->points + i2;

            /* 2D cross product (angle) between envelope edge and line */
            double a = (e1->x - e2->x) * (p1.y - p2.y) - (e1->y - e2->y) * (p1.x - p2.x);

            if (a > _PARALLEL_MARGIN) /* wrong direction */
                continue;
            else if (a > -_PARALLEL_MARGIN) { /* parallel, if lines are too close, try again */
                double dx = e1->x - p1.x;
                double dy = e1->y - p1.y;
                double d = nx * dx + ny * dy;
                if (d < _RETRY_MARGIN && d > -_RETRY_MARGIN) /* if lines are too close */
                    goto _RETRY;
                continue;
            }

            /* envelope edge fraction of intersection */
            double t = ((e1->x - p1.x) * (p1.y - p2.y) - (e1->y - p1.y) * (p1.x - p2.x)) / a;

            /* skip envelope edge if t is way off */
            if (t < -_RETRY_MARGIN || t > _ONE_PLUS_RETRY_MARGIN)
                continue;

            /* retry trace if intersection is too close to an envelope point */
            if (t < _RETRY_MARGIN || t > _ONE_MINUS_RETRY_MARGIN)
                goto _RETRY;

            p.i = i1;
            p.t = t;
            p.p.x = x;
            p.p.y = e1->x + (e2->x - e1->x) * t;
            p.p.z = e1->y + (e2->y - e1->y) * t;
            return p; // TODO: don't return here, find the farthest intersection in the line direction
        }

        _RETRY:;
    }

    return p;
}

/* Calculates intersection between wing edge and corresponding fuselage envelope
and stores the result in wing reference. */
static FuselageIsec _get_wing_root_point(Fuselage *fuselage, Arena *arena,
                                         Wref *wref, _Station *station,
                                         bool trailing) {

    TraceShapes *shapes = arena->lock<TraceShapes>();
    TraceEnv *env = arena->lock<TraceEnv>();

    /* fuselage envelope at station */

    fuselage_get_shapes_at_station(fuselage, station, shapes);
    mesh_trace_envelope(env,
                        trailing ? shapes->n_shapes : shapes->t_shapes,
                        trailing ? shapes->n_shapes_count : shapes->t_shapes_count,
                        SHAPE_CURVE_SAMPLES);

    /* ideal edge line, in the y-z plane */

    Wing *w = wref->wing;

    dvec p1;
    p1.x = (double)w->y;
    p1.y = (double)w->z;

    if (trailing) {
        p1.y += (double)(airfoil_get_trailing_y_offset(&w->airfoil) * w->chord);
        p1 = dvec_rotate(p1, (double)w->dihedral);
    }

    dvec p2;
    p2.x = p1.x + cos(w->dihedral * DEG_TO_RAD);
    p2.y = p1.y + sin(w->dihedral * DEG_TO_RAD);

    FuselageIsec isec = _envelope_and_line_intersection(env, p1, p2, station->x);

    arena->unlock();
    arena->unlock();

    return isec;
}

/* Why not just create the cutting prism, stored in arena and referenced from
wref? */
void fuselage_init_wing_cutter(Fuselage *fuselage, Arena *arena, Wref *wref,
                               _Station *t_station, _Station *n_station) {
    Wing *w = wref->wing;

    /* root points */

    wref->t_isec = _get_wing_root_point(fuselage, arena, wref, t_station, true);
    if (wref->t_isec.i == -1) /* could not find root point */
        return;

    wref->n_isec = _get_wing_root_point(fuselage, arena, wref, n_station, false);
    if (wref->n_isec.i == -1) /* could not find root point */
        return;

    tvec l1 = wref->t_isec.p;
    tvec t1 = wref->n_isec.p;

    /* tip points */

    tvec l2 = l1;
    l2.x -= sin(w->sweep * DEG_TO_RAD) * w->span;
    l2.y += cos(w->dihedral * DEG_TO_RAD) * w->span;
    l2.z += sin(w->dihedral * DEG_TO_RAD) * w->span;

    double tip_chord = (double)(w->chord * w->taper);
    double t2_y_off = airfoil_get_trailing_y_offset(&w->airfoil) * tip_chord;

    tvec t2 = l2;
    t2.x -= tip_chord;
    t2.y -= t2_y_off * sin(w->dihedral * DEG_TO_RAD);
    t2.z += t2_y_off * cos(w->dihedral * DEG_TO_RAD);

    /* make wing cutter prism */

    wref->r_cut = arena->alloc<tvec>(AIRFOIL_POINTS);
    wref->t_cut = arena->alloc<tvec>(AIRFOIL_POINTS);

    double s = sin(w->dihedral * DEG_TO_RAD);
    double c = cos(w->dihedral * DEG_TO_RAD);

    for (int i = 0; i < AIRFOIL_POINTS; ++i) {
        dvec p = airfoil_get_point(&w->airfoil, i);
        p.x *= w->chord;
        p.y *= w->chord;

        tvec *r = wref->r_cut + i;
        r->x = l1.x - p.x;
        r->y = l1.y - p.y * s;
        r->z = l1.z + p.y * c;

        tvec *t = wref->t_cut + i;
        t->x = l2.x - p.x;
        t->y = l2.y - p.y * s;
        t->z = l2.z + p.y * c;
    }
}

/* Adds points to envelope in-place. */
static void _insert_isecs_into_envelope(TraceEnv *env, FuselageIsec *isecs, int count) {
    if (count == 0)
        return;
    else if (count > 1) {
        // TODO: sort isecs
    }

    // ...
    // TODO: don't forget to remove points between two intersections
}

/* Uses wing reference's cutting prism to intersect given trace envelope.
Inserts intersection points into the envelope in-place. */
static int _intersect_envelope_with_wing(Wref *wref, _Station *station, TraceEnv *env, FuselageIsec *isecs) {

    /* check if station is on one of wing edges */

    if (flags_contains(&station->t_objs, wref->id)) { /* wing trailing edge station */
        isecs[0] = wref->t_isec;
        return 1;
    }
    else if (flags_contains(&station->n_objs, wref->id)) { /* wing leading edge station */
        isecs[0] = wref->n_isec;
        return 1;
    }

    if (station->x < wref->t_isec.p.x || station->x > wref->n_isec.p.x) /* station outside wing extents */
        return 0;

    /* TODO: intersect wing at station */

    // TODO: intersect wing intersection with envelope
    // use FuselageIsec _envelope_and_line_intersection(TraceEnv *env, dvec p1, dvec p2)

    return 0;
}
