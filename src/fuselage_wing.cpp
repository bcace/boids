#include "fuselage.h"
#include "wing.h"
#include "mesh.h"
#include "airfoil.h"
#include "periodic.h"
#include "dvec.h"
#include "arena.h"
#include "debug.h"
#include "math.h"
#include <math.h>

#define MAX_WING_SURFACE_STATIONS 500


static Wisec _envelope_and_line_intersection(TraceEnv *env, dvec p1, dvec p2, double x) {
    Wisec p;
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
static Wisec _get_wing_root_point(Wref *wref, TraceEnv *env, float x, bool trailing) {
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

    return _envelope_and_line_intersection(env, p1, p2, x);
}

static int _insert_wisec(Wisec *wisecs, int count, Wisec wisec) {

    int i = 0;
    for (; i < count; ++i) /* look for insertion point */
        if (wisec.i < wisecs[i].i ||
            wisec.i == wisecs[i].i && wisec.t < wisecs[i].t)
            break;

    for (int j = count; j > i; --j) /* make room */
        wisecs[j] = wisecs[j - 1];

    wisecs[i] = wisec; /* insert */

    return ++count;
}

static void _insert_wisecs_into_envelope(TraceEnv *env, Wisec *wisecs, int wisecs_count) {
    int last_loc = env->count - 1;
    while (wisecs_count > 0) {

        /* find wisecs batch to insert */
        int beg = wisecs_count - 1;
        int end = beg - 1;
        int loc = wisecs[beg].i;

        /* find end of wisecs batch */
        for (; end >= 0 && wisecs[end].i == loc; --end);
        int batch_count = beg - end;

        /* make room for insertion */
        for (int i = last_loc; i > loc; --i)
            env->points[i + wisecs_count] = env->points[i];

        wisecs_count -= batch_count;

        /* insert wisecs batch */
        EnvPoint *orig_p = env->points + loc;
        EnvPoint *base_p = env->points + loc + 1 + wisecs_count;
        for (int i = 0; i < batch_count; ++i) {
            Wisec *w = wisecs + i;
            EnvPoint *p = base_p + i;
            p->x = w->p.y;
            p->y = w->p.z;
            p->t1 = p->t2 = w->t;
            p->i1 = orig_p->i1;
            p->i2 = orig_p->i2;
            p->ids = orig_p->ids;
            p->subdiv_i = orig_p->subdiv_i;
            p->is_intersection = true;
        }

        last_loc = loc;
    }
}

void fuselage_wing_intersections(Arena *arena, Wref *wrefs, int wrefs_count, TraceSection *sects, int sects_count) {

    Wisec *u_wisecs = arena->lock<Wisec>(MAX_WING_SURFACE_STATIONS * 2);
    Wisec *l_wisecs = u_wisecs + MAX_WING_SURFACE_STATIONS;

    const int surf_points_count = AIRFOIL_X_SUBDIVS + 1;
    tvec *ru_surfs = arena->lock<tvec>(surf_points_count * 4);
    tvec *rl_surfs = ru_surfs + surf_points_count;
    tvec *tu_surfs = rl_surfs + surf_points_count;
    tvec *tl_surfs = tu_surfs + surf_points_count;

    /* find all intersections for each wing */

    for (int i = 0; i < wrefs_count; ++i) {
        Wref *wref = wrefs + i;
        Wing *w = wref->wing;

        wref->valid = true;

        TraceSection *t_sect = sects + wref->t_station.index;
        if (t_sect->t_env != t_sect->n_env) { /* possible opening */
            wref->valid = false;
            continue;
        }

        Wisec t_wisec = _get_wing_root_point(wref, t_sect->t_env, t_sect->x, true);
        if (t_wisec.i == -1) { /* intersection not found */
            wref->valid = false;
            continue;
        }

        TraceSection *n_sect = sects + wref->n_station.index;
        if (n_sect->t_env != n_sect->n_env) { /* possible opening */
            wref->valid = false;
            continue;
        }

        Wisec n_wisec = _get_wing_root_point(wref, n_sect->t_env, n_sect->x, false);
        if (n_wisec.i == -1) { /* intersection not found */
            wref->valid = false;
            continue;
        }

        /* tip points */

        tvec l1 = t_wisec.p;
        tvec t1 = n_wisec.p;

        double s = sin(w->dihedral * DEG_TO_RAD);
        double c = cos(w->dihedral * DEG_TO_RAD);

        tvec l2 = l1;
        l2.x -= sin(w->sweep * DEG_TO_RAD) * w->span;
        l2.y += c * w->span;
        l2.z += s * w->span;

        double tip_chord = (double)(w->chord * w->taper);
        double t2_y_off = airfoil_get_trailing_y_offset(&w->airfoil) * tip_chord;

        tvec t2 = l2;
        t2.x -= tip_chord;
        t2.y -= t2_y_off * s;
        t2.z += t2_y_off * c;

        /* make wing cutter prism */
        // for (int j = 0; j < AIRFOIL_X_SUBDIVS; ++j) {
        //     double x = airfoil_get_subdiv_x(AIRFOIL_X_SUBDIVS - j);
        // }

        // for (int j = 0; j < AIRFOIL_POINTS; ++j) {
        //     dvec p = airfoil_get_point(&w->airfoil, j);
        //     p.x *= w->chord;
        //     p.y *= w->chord;

        //     tvec *r = r_cut + j;
        //     r->x = l1.x - p.x;
        //     r->y = l1.y - p.y * s;
        //     r->z = l1.z + p.y * c;

        //     tvec *t = t_cut + j;
        //     t->x = l2.x - p.x;
        //     t->y = l2.y - p.y * s;
        //     t->z = l2.z + p.y * c;
        // }

        /* wing surface intersections */

        int surface_stations_count = wref->n_station.index - wref->t_station.index - 1;
        break_assert(surface_stations_count <= MAX_WING_SURFACE_STATIONS);

        for (int j = wref->t_station.index + 1; j < wref->n_station.index; ++j) {
            TraceSection *sect = sects + j;

            /* TODO: find cutting prism intersection at sect->x
            intersect this intersection with section envelope */
        }

        /* insert found wisecs here */
        t_sect->wisecs_count = _insert_wisec(t_sect->wisecs, t_sect->wisecs_count, t_wisec);
        n_sect->wisecs_count = _insert_wisec(n_sect->wisecs, n_sect->wisecs_count, n_wisec);
        // for (int j = 0; j < surf_points_count; ++j) {
        //     t_sect->wisecs_count = _insert_wisec(t_sect->wisecs, t_sect->wisecs_count, t_wisecs[j]);
        //     n_sect->wisecs_count = _insert_wisec(n_sect->wisecs, n_sect->wisecs_count, n_wisecs[j]);
        // }
    }

    arena->unlock();
    arena->unlock();

    /* check wisecs for overlap */

    for (int i = 0; i < sects_count; ++i) {
        TraceSection *sect = sects + i;

        // ...
    }

    // TODO: prune wisecs of invalidated wrefs

    // TODO: insert remaining wisecs into envelopes

    for (int i = 0; i < sects_count; ++i) {
        TraceSection *sect = sects + i;
        _insert_wisecs_into_envelope(sect->t_env, sect->wisecs, sect->wisecs_count);
    }
}
