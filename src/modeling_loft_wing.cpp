#include "modeling_loft.h"
#include "modeling_wing.h"
#include "modeling_airfoil.h"
#include "math_periodic.h"
#include "math_dvec.h"
#include "math_math.h"
#include "memory_arena.h"
#include <float.h>
#include <math.h>

#define MAX_WING_SURFACE_STATIONS 500


static dvec _line_intersection(double x1, double y1, double x2, double y2,
                               double x3, double y3, double x4, double y4) {
    double d =  (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / d;
    dvec i;
    i.x = x1 + (x2 - x1) * t;
    i.y = y1 + (y2 - y1) * t;
    return i;
}

static Wisec _envelope_and_line_intersection(TraceEnv *env, double x1, double y1, double x2, double y2, double x) {
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
    double nx = y1 - y2;
    double ny = x2 - x1;
    double nl = sqrt(nx * nx + ny * ny);
    nx /= nl;
    ny /= nl;

    for (int try_i = 0; try_i < _MAX_TRIES; ++try_i) {

        /* if retrying move line by retry step in the direction of its normal */
        if (try_i) {
            double dx = nx * _RETRY_STEP * try_i;
            double dy = ny * _RETRY_STEP * try_i;
            x1 += dx;
            y1 += dy;
            x2 += dx;
            y2 += dy;
        }

        for (int i1 = 0; i1 < env->count; ++i1) {
            int i2 = period_incr(i1, env->count);
            EnvPoint *e1 = env->points + i1;
            EnvPoint *e2 = env->points + i2;

            /* 2D cross product (angle) between envelope edge and line */
            double a = (e1->x - e2->x) * (y1 - y2) - (e1->y - e2->y) * (x1 - x2);

            if (a > _PARALLEL_MARGIN) /* wrong direction */
                continue;
            else if (a > -_PARALLEL_MARGIN) { /* parallel, if lines are too close, try again */
                double dx = e1->x - x1;
                double dy = e1->y - y1;
                double d = nx * dx + ny * dy;
                if (d < _RETRY_MARGIN && d > -_RETRY_MARGIN) /* if lines are too close */
                    goto _RETRY;
                continue;
            }

            /* envelope edge fraction of intersection */
            double t = ((e1->x - x1) * (y1 - y2) - (e1->y - y1) * (x1 - x2)) / a;

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

// static double _impart_aoa_and_chord(double x, double y, double aoa, double chord) {
//     return y * cos(aoa) * chord - x * sin(aoa) * chord;
// }

// /* Calculates intersection between wing edge and corresponding fuselage envelope. */
// static Wisec _get_wing_root_point(Wref *wref, TraceEnv *env, float x, bool trailing) {
//     Wing *w = wref->wing;

//     double x1 = (double)w->y;
//     double y1 = (double)w->z;

//     WFormer *r_form = &wref->wing->def.r_former;
//     WFormer *t_form = &wref->wing->def.t_former;
//     double dihedral = atan2(t_form->z - r_form->z, t_form->y - r_form->y);

//     if (trailing) {
//         /* airfoil CS */
//         // y1 += (double)(dy * cos(r_form->aoa) - dx * sin(r_form->aoa));
//         y1 += _impart_aoa_and_chord(1.0, airfoil_get_trailing_y_offset(&r_form->airfoil), r_form->aoa, r_form->chord);
//     }

//     double x2 = x1 + cos(dihedral);
//     double y2 = y1 + sin(dihedral);

//     return _envelope_and_line_intersection(env, x1, y1, x2, y2, x);
// }

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

#if 0
static tvec _get_direction(tvec v1, tvec v2) {
    tvec d;
    d.x = v2.x - v1.x;
    d.y = v2.y - v1.y;
    d.z = v2.z - v1.z;
    double l = sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    d.x /= l;
    d.y /= l;
    d.z /= l;
    return d;
}

struct Edges {
    struct {
        tvec root, tip, dir;
    } tr, le;
};

static void _get_wing_section(tvec l1, tvec l2, tvec ld, tvec t1, tvec t2, tvec td,
                              double l_dist, double airfoil_dy,
                              tvec *verts) {

    tvec lp; /* leading edge position */
    lp.x = l1.x + ld.x * l_dist;
    lp.y = l1.y + ld.y * l_dist;
    lp.z = l1.z + ld.z * l_dist;

    dvec p = _line_intersection(t1.y, t1.z, t2.y, t2.z,
                                lp.y, lp.z, lp.y - ld.z, lp.z + ld.y);

    double t;
    if (fabs(td.z) > fabs(td.y))
        t = (p.y - t1.z) / (t2.z - t1.z);
    else
        t = (p.x - t1.y) / (t2.y - t1.y);

    tvec tp; /* trailing edge position */
    tp.x = t1.x + td.x * t;
    tp.y = t1.y + td.y * t;
    tp.z = t1.z + td.z * t;

    /* connecting line between trailing and leading edge positions */
    double cx = lp.x - tp.x;
    double cy = lp.y - tp.y;
    double cz = lp.z - tp.z;

    double aoa = asin(cx / sqrt(cy * cy + cz * cz)) - asin(1.0 / airfoil_dy);
    double chord = sqrt(cx * cx + cy * cy + cz * cz) / sqrt(1.0 + airfoil_dy * airfoil_dy);

    // TODO: make vertices
}
#endif

tvec _interp_wing_section_surface(tvec *verts, double x, int *beg) {
    while (verts[*beg + 1].x < x)
        ++(*beg);
    double t = (x - verts[*beg].x) / (verts[*beg + 1].x - verts[*beg].x);
    tvec s;
    s.x = x;
    s.y = verts[*beg].y + (verts[*beg + 1].y - verts[*beg].y) * t;
    s.z = verts[*beg].z + (verts[*beg + 1].z - verts[*beg].z) * t;
    return s;
}

void loft_fuselage_wing_intersections(Arena *arena, Wref *wrefs, int wrefs_count, TraceSection *sects, int sects_count) {

    tvec *u_verts = arena->lock<tvec>(AIRFOIL_X_SUBDIVS + 1);
    tvec *l_verts = arena->lock<tvec>(AIRFOIL_X_SUBDIVS + 1);

    /* find fuselage intersections for each wing */

    for (int i = 0; i < wrefs_count; ++i) {
        Wref *wref = wrefs + i;
        Wing *w = wref->wing;

        /* find tailmost and nosemost intersection points */

        wref->isec_valid = true;

        TraceSection *t_sect = sects + wref->t_station.index;
        if (t_sect->t_env != t_sect->n_env) { /* possible opening */
            wref->isec_valid = false;
            continue;
        }

        TraceSection *n_sect = sects + wref->n_station.index;
        if (n_sect->t_env != n_sect->n_env) { /* possible opening */
            wref->isec_valid = false;
            continue;
        }

        /* find how much the airfoil has to be scaled to fit the given chord */

        WFormer *r_form = &wref->wing->def.r_former;
        WFormer *t_form = &wref->wing->def.t_former;
        Airfoil *r_airfoil = &r_form->airfoil;

        double nominal_c = (double)wing_get_nominal_root_chord(w);
        double a1 = atan2((double)airfoil_get_trailing_y_offset(r_airfoil), nominal_c);
        double a2 = a1 + w->def.r_former.aoa;
        double scaled_c = nominal_c * cos(a1) / cos(a2);
        double dihedral = atan2(t_form->z - r_form->z, t_form->y - r_form->y);

        airfoil_get_verts(r_airfoil, u_verts, l_verts, dihedral, scaled_c, r_form->aoa,
                          w->x + r_form->x, w->y + r_form->y, w->z + r_form->z);

        double cd = cos(dihedral);
        double sd = sin(dihedral);

        /* trailing edge point */

        Wisec t_wisec = _envelope_and_line_intersection(t_sect->t_env,
            u_verts[0].y, u_verts[0].z,
            u_verts[0].y + cd, u_verts[0].z + sd,
            t_sect->x);

        if (t_wisec.i == -1) {
            wref->isec_valid = false;
            continue;
        }

        t_sect->wisecs_count = _insert_wisec(t_sect->wisecs, t_sect->wisecs_count, t_wisec);

        /* surface points */

        int u_i = 0;

        for (int j = wref->t_station.index + 1; j < wref->n_station.index; ++j) {
            TraceSection *sect = sects + j;

            tvec u_vert = _interp_wing_section_surface(u_verts, sect->x, &u_i);

            Wisec u_wisec = _envelope_and_line_intersection(sect->t_env,
                u_vert.y, u_vert.z,
                u_vert.y + cd, u_vert.z + sd,
                sect->x);

            if (u_wisec.i == -1) {
                wref->isec_valid = false;
                break;
            }

            sect->wisecs_count = _insert_wisec(sect->wisecs, sect->wisecs_count, u_wisec);
        }

        if (!wref->isec_valid)
            continue;

        /* leading edge point */

        Wisec n_wisec = _envelope_and_line_intersection(n_sect->t_env,
            u_verts[AIRFOIL_X_SUBDIVS].y, u_verts[AIRFOIL_X_SUBDIVS].z,
            u_verts[AIRFOIL_X_SUBDIVS].y + cd, u_verts[AIRFOIL_X_SUBDIVS].z + sd,
            t_sect->x);

        if (n_wisec.i == -1) {
            wref->isec_valid = false;
            continue;
        }

        n_sect->wisecs_count = _insert_wisec(n_sect->wisecs, n_sect->wisecs_count, n_wisec);
    }

    /* for wings that intersect fuselages correctly add insert all its intersection points
    into trace envelopes */

    for (int i = 0; i < sects_count; ++i) {
        TraceSection *sect = sects + i;
        _insert_wisecs_into_envelope(sect->t_env, sect->wisecs, sect->wisecs_count);
    }

    arena->unlock();
    arena->unlock();
}


#if 0
    Wisec *u_wisecs = arena->lock<Wisec>(MAX_WING_SURFACE_STATIONS * 2);
    Wisec *l_wisecs = u_wisecs + MAX_WING_SURFACE_STATIONS;

    tvec *r_prof = arena->lock<tvec>(AIRFOIL_POINTS * 2);
    tvec *t_prof = r_prof + AIRFOIL_POINTS;

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

        /* find tip points, leading edge a-b, trailing edge d-c
        ...
        */

        tvec t1 = t_wisec.p;
        tvec l1 = n_wisec.p;

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

        /* go through all wing sections and find min/max for leading edge y-z direction */
        double r_margin =  DBL_MAX;
        double t_margin = -DBL_MAX;
        {
            double nx = l2.y - l1.y;
            double ny = l2.z - l1.z;
            for (int j = wref->t_station.index + 1; j < wref->n_station.index; ++j) {
                TraceEnv *env = sects[j].t_env;
                for (int k = 0; k < env->count; ++k) {
                    double d = nx * (env->points[k].x - l1.y) + ny * (env->points[k].y - l1.z);
                    if (d < r_margin)
                        r_margin = d;
                    if (d > t_margin)
                        t_margin = d;
                }
            }
            r_margin -= 0.01;
            t_margin += 0.01;
        }

        double airfoil_dy = airfoil_get_trailing_y_offset(&w->airfoil);
        tvec ld = _get_direction(l1, l2);
        tvec td = _get_direction(t1, t2);

        _get_wing_section(l1, l2, ld, t1, t2, td, r_margin, airfoil_dy, r_prof);
        _get_wing_section(l1, l2, ld, t1, t2, td, t_margin, airfoil_dy, t_prof);

        /* wing surface intersections */

        // int surface_stations_count = wref->n_station.index - wref->t_station.index - 1;
        // break_assert(surface_stations_count <= MAX_WING_SURFACE_STATIONS);

        // for (int j = wref->t_station.index + 1; j < wref->n_station.index; ++j) {
        //     TraceSection *sect = sects + j;
        // }

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
#endif
