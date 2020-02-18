#include "mesh.h"
#include "shape.h"
#include "model.h"
#include "debug.h"
#include "periodic.h"
#include "arena.h"
#include "config.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


static Arena mesh_arena(50000000);

#if DRAW_CORRS
#include "vec.h"

static Arena corr_verts_arena(1000000);
static Arena corr_colors_arena(1000000);
static Model *ctx_model;
static float ctx_x1, ctx_x2;

void _init_corr(Model *model, float x1, float x2) {
    ctx_model = model;
    ctx_x1 = x1;
    ctx_x2 = x2;
}

void _draw_corr(MeshPoint *t_p, MeshPoint *n_p, float r, float g, float b) {
    vec3 *v = corr_verts_arena.alloc<vec3>(2);
    v->x = ctx_x1;
    v->y = (float)t_p->x;
    v->z = (float)t_p->y;
    ++v;
    v->x = ctx_x2;
    v->y = (float)n_p->x;
    v->z = (float)n_p->y;
    vec3 *c = corr_colors_arena.alloc<vec3>(2);
    c->r = r;
    c->g = g;
    c->b = b;
    ++c;
    c->r = r;
    c->g = g;
    c->b = b;
    ++ctx_model->corrs_count;
}

#endif

/* Initialize. */
void mesh_init(Model *model) {
    mesh_arena.clear();
    model->panels = mesh_arena.rest<Panel>();
    model->panels_count = 0;

#if DRAW_CORRS
    corr_verts_arena.clear();
    corr_colors_arena.clear();
    model->corr_verts = corr_verts_arena.rest<vec3>();
    model->corr_colors = corr_colors_arena.rest<vec3>();
    model->corrs_count = 0;
#endif
}

// TODO: maybe bundle this somewhere with Origin
static bool _origins_related(Origin tail_origin, Origin nose_origin) {
    if (tail_origin.nose == nose_origin.tail) /* two shapes of the same object, one object and one connection shape */
        return true;
    else if (tail_origin.tail == nose_origin.tail && tail_origin.nose == nose_origin.nose) /* two shapes of the same connection */
        return true;
    return false;
}

/* Add a single skin panel and connect neighbors. */
static void _add_skin_panel(Model *m,
                            int prev_t_env_i, int next_t_env_i, MeshEnv *t_env, int *t_neighbors_map,
                            int prev_n_env_i, int next_n_env_i, MeshEnv *n_env, int *n_neighbors_map) {

    int t_i1 = t_env->points[prev_t_env_i].vert_i;
    int n_i1 = n_env->points[prev_n_env_i].vert_i;

    /* set panel vertices */

    Panel *p = mesh_arena.alloc<Panel>(1);
    if (next_t_env_i == -1) {       /* tailwise triangle */
        int n_i2 = n_env->points[next_n_env_i].vert_i;
        p->v1 = t_i1 + t_env->verts_base_i;
        p->v2 = n_i2 + n_env->verts_base_i;
        p->v3 = n_i1 + n_env->verts_base_i;
        p->v4 = -1;
    }
    else if (next_n_env_i == -1) {  /* nosewise triangle */
        int t_i2 = t_env->points[next_t_env_i].vert_i;
        p->v1 = t_i1 + t_env->verts_base_i;
        p->v2 = t_i2 + t_env->verts_base_i;
        p->v3 = n_i1 + n_env->verts_base_i;
        p->v4 = -1;
    }
    else {                          /* quad */
        int t_i2 = t_env->points[next_t_env_i].vert_i;
        int n_i2 = n_env->points[next_n_env_i].vert_i;
        p->v1 = t_i1 + t_env->verts_base_i;
        p->v2 = t_i2 + t_env->verts_base_i;
        p->v3 = n_i2 + n_env->verts_base_i;
        p->v4 = n_i1 + n_env->verts_base_i;
    }

    /* set panel neighbors */

    // p->prev = m->panels_count - 1;
    // p->next = m->panels_count + 1;
    // p->tail = (next_t_env_i != -1) ? t_neighbors_map[t_i1] : -1;    /* set tailwise neighbor if this panel can have it (quad or nosewise triangle) */
    // p->nose = -1;                                                   /* nosewise neighbor will (maybe) be set in future */
    // if (p->tail != -1)                                              /* if this panel has a tailwise neighbor */
    //     m->panels[p->tail].nose = m->panels_count;                  /* set it as its nosewise neighbor (set back-reference) */
    // if (next_n_env_i != -1)                                         /* if this panel can be a tailwise neighbor (quad or tailwise triangle) */
    //     n_neighbors_map[n_i1] = m->panels_count;                    /* remember it as tailwise panel to something in future */

    ++m->panels_count;
}

/* Returns early or subdivides points into two sides wrt normalized positions and recurses on both sides. */
static void _mesh_pass_5(Model *model,
                         int t_beg, int t_end, double *t_params, MeshEnv *t_env, int *t_neighbors_map,
                         int n_beg, int n_end, double *n_params, MeshEnv *n_env, int *n_neighbors_map) {

    /* special cases for early-out */

    int t_count = period_range_count(t_beg, t_end, t_env->count);
    int n_count = period_range_count(n_beg, n_end, n_env->count);

    if (t_count == 2 && n_count == 2) { /* quad */

        // TODO: think about splitting this into two triangles if angle between two quad sides is too large

        _add_skin_panel(model,
                        t_beg, t_end, t_env, t_neighbors_map,
                        n_beg, n_end, n_env, n_neighbors_map);
        return;
    }
    else if (n_count == 1) { /* nosewise triangle (fan) */
        int i1 = t_beg;
        int i2 = period_incr(t_beg, t_env->count);
        while (i1 != t_end) {
            _add_skin_panel(model,
                            i1,    i2, t_env, t_neighbors_map,
                            n_beg, -1, n_env, n_neighbors_map);
            i1 = i2;
            i2 = period_incr(i2, t_env->count);
        }
        return;
    }
    else if (t_count == 1) { /* tailwise triangle (fan) */
        int i1 = n_beg;
        int i2 = period_incr(n_beg, n_env->count);
        while (i1 != n_end) {
            _add_skin_panel(model,
                            t_beg, -1, t_env, t_neighbors_map,
                            i1,    i2, n_env, n_neighbors_map);
            i1 = i2;
            i2 = period_incr(i2, n_env->count);
        }
        return;
    }

    /* divide vertices into two sides by finding the closest pair */

    int t_min = -1, n_min = -1;
    double min_dh = DBL_MAX;
    int t_sentry = period_incr(t_end, t_env->count);
    int n_sentry = period_incr(n_end, n_env->count);

    for (int t_i = t_beg; t_i != t_sentry; t_i = period_incr(t_i, t_env->count)) {
        for (int n_i = n_beg; n_i != n_sentry; n_i = period_incr(n_i, n_env->count)) {
            if ((t_i != t_beg || n_i != n_beg) && (t_i != t_end || n_i != n_end)) { /* beg and end points are already connected */
                double dh = fabs(t_params[t_i] - n_params[n_i]);
                if (dh < min_dh) {
                    min_dh = dh;
                    t_min = t_i;
                    n_min = n_i;
                }
            }
        }
    }

    if (t_min == -1)
        return;

    model_assert(model, t_min != -1, "cannot_find_divider_in_mesh_pass_5");

    /* mesh both resulting sides */

    _mesh_pass_5(model,
                 t_beg, t_min, t_params, t_env, t_neighbors_map,
                 n_beg, n_min, n_params, n_env, n_neighbors_map);
    _mesh_pass_5(model,
                 t_min, t_end, t_params, t_env, t_neighbors_map,
                 n_min, n_end, n_params, n_env, n_neighbors_map);
}

/* Calculate normalized positions of a range of envelope points. */
static void _normalized_positions(MeshPoint *env_points, int env_count, int beg, int end, double *params) {
    double l = 0.0;
    params[beg] = 0.0;

    int i1 = beg;
    int i2 = period_incr(beg, env_count);
    while (i1 != end) {
        double dx = env_points[i2].x - env_points[i1].x;
        double dy = env_points[i2].y - env_points[i1].y;
        double dl = sqrt(dx * dx + dy * dy); // FAST_SQRT
        params[i2] = (l += dl);
        i1 = i2;
        i2 = period_incr(i2, env_count);
    }

    for (int i = beg; i != end; i = period_incr(i, env_count))
        params[i] /= l;
    params[end] = 1.0;
}

/* Handles non-intersections merging into intersection edges. */
static void _mesh_pass_4(Model *model,
                         int t_beg, int t_end, MeshEnv *t_env, int *t_neighbors_map,
                         int n_beg, int n_end, MeshEnv *n_env, int *n_neighbors_map) {
    static double t_params[SHAPE_MAX_ENVELOPE_POINTS];
    static double n_params[SHAPE_MAX_ENVELOPE_POINTS];
    _normalized_positions(t_env->points, t_env->count, t_beg, t_end, t_params);
    _normalized_positions(n_env->points, n_env->count, n_beg, n_end, n_params);

    int t_count = period_range_count(t_beg, t_end, t_env->count);
    int n_count = period_range_count(n_beg, n_end, n_env->count);

    MeshPoint *t_beg_p = t_env->points + t_beg;
    MeshPoint *t_end_p = t_env->points + t_end;
    MeshPoint *n_beg_p = n_env->points + n_beg;
    MeshPoint *n_end_p = n_env->points + n_end;

    int shape_subdivs = SHAPE_CURVE_SAMPLES * SHAPE_CURVES;

    if (t_beg_p->is_intersection && n_beg_p->is_intersection) { /* beg edge is intersection */
        int isec_diff = period_diff(n_beg_p->i2, t_beg_p->i2, shape_subdivs);

        if (isec_diff > 0) { /* nosewise triangle fan expected */
            if (t_count > 2) {
                int t_i = period_incr(t_beg, t_env->count);
                MeshPoint *t_p = t_env->points + t_i;

                while (t_i != t_end && period_diff(n_beg_p->i2, t_p->i1, shape_subdivs) >= 0) {
                    _add_skin_panel(model,
                                    t_beg, t_i, t_env, t_neighbors_map,
                                    n_beg,  -1, n_env, n_neighbors_map);
                    t_beg = t_i;
                    t_i = period_incr(t_i, t_env->count);
                    t_p = t_env->points + t_i;
                }
            }
        }
        else if (isec_diff < 0) { /* tailwise triangle fan expected */
            if (n_count > 2) {
                int n_i = period_incr(n_beg, n_env->count);
                MeshPoint *n_p = n_env->points + n_i;

                while (n_i != n_end && period_diff(t_beg_p->i2, n_p->i1, shape_subdivs) >= 0) {
                    _add_skin_panel(model,
                                    t_beg,  -1, t_env, t_neighbors_map,
                                    n_beg, n_i, n_env, n_neighbors_map);
                    n_beg = n_i;
                    n_i = period_incr(n_i, n_env->count);
                    n_p = n_env->points + n_i;
                }
            }
        }
    }

    if (t_end_p->is_intersection && n_end_p->is_intersection) { /* end edge is intersection */
        int isec_diff = period_diff(n_end_p->i1, t_end_p->i1, shape_subdivs);

        if (isec_diff > 0) { /* tailwise triangle fan expected */
            if (n_count > 2) {
                int n_i = period_decr(n_end, n_env->count);
                MeshPoint *n_p = n_env->points + n_i;

                while (n_i != n_beg && period_diff(n_p->i2, t_end_p->i1, shape_subdivs) >= 0) {
                    _add_skin_panel(model,
                                    t_end,  -1, t_env, t_neighbors_map,
                                    n_i, n_end, n_env, n_neighbors_map);
                    n_end = n_i;
                    n_i = period_decr(n_i, n_env->count);
                    n_p = n_env->points + n_i;
                }
            }
        }
        else if (isec_diff < 0) { /* nosewise triangle fan expected */
            if (t_count > 2) {
                int t_i = period_decr(t_end, t_env->count);
                MeshPoint *t_p = t_env->points + t_i;

                while (t_i != t_beg && period_diff(t_p->i2, n_end_p->i1, shape_subdivs) >= 0) {
                    _add_skin_panel(model,
                                    t_i, t_end, t_env, t_neighbors_map,
                                    n_end,  -1, n_env, n_neighbors_map);
                    t_end = t_i;
                    t_i = period_decr(t_i, t_env->count);
                    t_p = t_env->points + t_i;
                }
            }
        }
    }

    _mesh_pass_5(model,
                 t_beg, t_end, t_params, t_env, t_neighbors_map,
                 n_beg, n_end, n_params, n_env, n_neighbors_map);
}

/* Additional intersection info used for intersection correlation. */
struct _Isec {
    int env_i;
    Origin prev_o, next_o;
};

/* Handles intersection merges (two-to-one intersection correlations). */
static void _mesh_pass_3(Model *model,
                         int t_beg, int t_end, MeshEnv *t_env, int *t_neighbors_map,
                         int n_beg, int n_end, MeshEnv *n_env, int *n_neighbors_map,
                         _Isec *t_isecs, int t_beg_isec, int t_end_isec,
                         _Isec *n_isecs, int n_beg_isec, int n_end_isec) {

    int t_count = t_end_isec - t_beg_isec - 1;
    int n_count = n_end_isec - n_beg_isec - 1;
    bool merge_found = false;

    break_assert(t_count >= 0 && n_count >= 0);

    if (t_count == 1 && n_count == 2) { /* (possible) tailwise intersection merge */
        _Isec *t_isec = t_isecs + t_beg_isec + 1;
        _Isec *n_isec1 = n_isecs + n_beg_isec + 1;
        _Isec *n_isec2 = n_isecs + n_beg_isec + 2;

        if (_origins_related(t_isec->prev_o, n_isec1->prev_o) &&
            _origins_related(t_isec->next_o, n_isec2->next_o) &&
            _origins_related(n_isec1->next_o, n_isec2->prev_o)) {

            _mesh_pass_4(model,
                         t_beg, t_isec->env_i, t_env, t_neighbors_map,
                         n_beg, n_isec1->env_i, n_env, n_neighbors_map);
            _mesh_pass_4(model,
                         t_isec->env_i, t_isec->env_i, t_env, t_neighbors_map,
                         n_isec1->env_i, n_isec2->env_i, n_env, n_neighbors_map);
            _mesh_pass_4(model,
                         t_isec->env_i, t_end, t_env, t_neighbors_map,
                         n_isec2->env_i, n_end, n_env, n_neighbors_map);

#if DRAW_CORRS
            _draw_corr(t_env->points + t_isec->env_i, n_env->points + n_isec1->env_i, 0, 1, 0);
            _draw_corr(t_env->points + t_isec->env_i, n_env->points + n_isec2->env_i, 0, 1, 0);
#endif

            merge_found = true;
        }
    }
    else if (t_count == 2 && n_count == 1) { /* (possible) nosewise intersection merge */
        _Isec *n_isec = n_isecs + n_beg_isec + 1;
        _Isec *t_isec1 = t_isecs + t_beg_isec + 1;
        _Isec *t_isec2 = t_isecs + t_beg_isec + 2;

        if (_origins_related(n_isec->prev_o, t_isec1->prev_o) &&
            _origins_related(n_isec->next_o, t_isec2->next_o) &&
            _origins_related(t_isec1->next_o, t_isec2->prev_o)) {

            _mesh_pass_4(model,
                         t_beg, t_isec1->env_i, t_env, t_neighbors_map,
                         n_beg, n_isec->env_i, n_env, n_neighbors_map);
            _mesh_pass_4(model,
                         t_isec1->env_i, t_isec2->env_i, t_env, t_neighbors_map,
                         n_isec->env_i, n_isec->env_i, n_env, n_neighbors_map);
            _mesh_pass_4(model,
                         t_isec2->env_i, t_end, t_env, t_neighbors_map,
                         n_isec->env_i, n_end, n_env, n_neighbors_map);

#if DRAW_CORRS
            _draw_corr(t_env->points + t_isec1->env_i, n_env->points + n_isec->env_i, 0, 1, 0);
            _draw_corr(t_env->points + t_isec2->env_i, n_env->points + n_isec->env_i, 0, 1, 0);
#endif

            merge_found = true;
        }
    }

    if (!merge_found) /* no intersection merge found, proceed to next pass */
        _mesh_pass_4(model,
                     t_beg, t_end, t_env, t_neighbors_map,
                     n_beg, n_end, n_env, n_neighbors_map);
}

/* One-to-one correlate intersections and continue meshing in between. */
static void _mesh_pass_2(Model *model,
                         MeshEnv *t_env, int prev_t_i, int last_t_i, int *t_neighbors_map,
                         MeshEnv *n_env, int prev_n_i, int last_n_i, int *n_neighbors_map)  {
    Origin t_prev_o = t_env->points[prev_t_i].origin;
    Origin n_prev_o = n_env->points[prev_n_i].origin;

    static _Isec t_isecs[SHAPE_MAX_ENVELOPE_POINTS]; /* envelope indices */
    int t_isecs_count = 0;

    for (int t_i = period_incr(prev_t_i, t_env->count); t_i != last_t_i; t_i = period_incr(t_i, t_env->count))
        if (t_env->points[t_i].is_intersection) {
            _Isec *isec = t_isecs + t_isecs_count++;
            isec->env_i = t_i;
            isec->prev_o = t_prev_o;
            isec->next_o = t_prev_o = t_env->points[t_i].origin;
        }

    static _Isec n_isecs[SHAPE_MAX_ENVELOPE_POINTS]; /* envelope indices */
    int n_isecs_count = 0;

    for (int n_i = period_incr(prev_n_i, n_env->count); n_i != last_n_i; n_i = period_incr(n_i, n_env->count))
        if (n_env->points[n_i].is_intersection) {
            _Isec *isec = n_isecs + n_isecs_count++;
            isec->env_i = n_i;
            isec->prev_o = n_prev_o;
            isec->next_o = n_prev_o = n_env->points[n_i].origin;
        }

    int prev_t_j = -1;
    int prev_n_j = -1;

    for (int t_j = prev_t_j + 1; t_j < t_isecs_count; ++t_j) {
        _Isec *t_isec = t_isecs + t_j;
        int t_i = t_isec->env_i;

        for (int n_j = prev_n_j + 1; n_j < n_isecs_count; ++n_j) {
            _Isec *n_isec = n_isecs + n_j;
            int n_i = n_isec->env_i;

            if (_origins_related(t_isec->prev_o, n_isec->prev_o) &&
                _origins_related(t_isec->next_o, n_isec->next_o)) {

                _mesh_pass_3(model,
                             prev_t_i, t_i, t_env, t_neighbors_map,
                             prev_n_i, n_i, n_env, n_neighbors_map,
                             t_isecs, prev_t_j, t_j,
                             n_isecs, prev_n_j, n_j);

#if DRAW_CORRS
                _draw_corr(t_env->points + t_isec->env_i, n_env->points + n_isec->env_i, 0, 0, 1);
#endif

                prev_t_i = t_i;
                prev_n_i = n_i;
                prev_t_j = t_j;
                prev_n_j = n_j;
                break; /* found nose isec that matches the current tail isec, stop iterating nose isecs */
            }
        }
    }

    /* check if there were some interesting intersections stuck between the last correlated ones */

    _mesh_pass_3(model,
                 prev_t_i, last_t_i, t_env, t_neighbors_map,
                 prev_n_i, last_n_i, n_env, n_neighbors_map,
                 t_isecs, prev_t_j, t_isecs_count,
                 n_isecs, prev_n_j, n_isecs_count);
}

/* Non-intersection point correlation struct. */
static struct _Corr {
    int t_env_i;
    int n_env_i;
} *corrs = 0;

/* Make simple correlations between non-intersection points. */
static void _mesh_pass_1(Model *model, int shape_subdivs,
                         MeshEnv *t_env, int *t_neighbors_map,
                         MeshEnv *n_env, int *n_neighbors_map) {

    if (corrs == 0)
        corrs = (_Corr *)malloc(sizeof(_Corr) * SHAPE_MAX_ENVELOPE_POINTS);
    int corrs_count = 0;

    /* make non-intersection point correlations */

    for (int j = 0; j < t_env->slices_count; ++j) {
        MeshEnvSlice *t_slice = t_env->slices + j;

        for (int k = 0; k < n_env->slices_count; ++k) {
            MeshEnvSlice *n_slice = n_env->slices + k;

            if (!_origins_related(t_slice->origin, n_slice->origin))
                continue;

            int n_poly_beg = n_env->points[n_slice->beg].subdiv_i;
            int n_poly_end = n_env->points[n_slice->end].subdiv_i;

            for (int l = 0; l < SHAPE_MAX_ENVELOPE_POINTS; ++l) {
                int t_env_i = (t_slice->beg + l) % t_env->count;
                MeshPoint *t_p = t_env->points + t_env_i;

                if (t_p->n_is_outermost) {
                    int t_poly_i = t_p->subdiv_i;

                    int offset = period_offset_if_contains(t_poly_i, n_poly_beg, n_poly_end, shape_subdivs);
                    if (offset != -1) {
                        int n_env_i = (n_slice->beg + offset) % n_env->count;
                        MeshPoint *n_p = n_env->points + n_env_i;

                        if (n_p->t_is_outermost) {

                            int insert_i = 0;
                            for (; insert_i < corrs_count; ++insert_i) // TODO: maybe think about starting search from end
                                if (t_env_i < corrs[insert_i].t_env_i ||
                                    t_env_i == corrs[insert_i].t_env_i && n_env_i < corrs[insert_i].n_env_i)
                                    break;
                            for (int m = corrs_count; m > insert_i; --m)
                                corrs[m] = corrs[m - 1];

                            _Corr *corr = corrs + insert_i;
                            corr->t_env_i = t_env_i;
                            corr->n_env_i = n_env_i;
                            ++corrs_count;

#if DRAW_CORRS
                            _draw_corr(t_env->points + t_env_i, n_env->points + n_env_i, 1.0f, 0.0f, 0.0f);
#endif
                        }
                    }
                }

                if (t_env_i == t_slice->end)
                    break;
            }
        }
    }

    /* make panels out of correlations if they're adjacent, otherwise proceed to further correlation steps */

    _Corr prev_corr = corrs[corrs_count - 1];
    for (int j = 0; j < corrs_count; ++j) {
        _Corr curr_corr = corrs[j];

        int t_count = period_range_count(prev_corr.t_env_i, curr_corr.t_env_i, t_env->count);
        int n_count = period_range_count(prev_corr.n_env_i, curr_corr.n_env_i, n_env->count);

        if (t_count == 1 && n_count == 1)           /* overlapping correlations, should not happen */
            fprintf(stderr, "overlapping correlations at section\n");
        else if (t_count == 2 && n_count == 2)      /* exactly one quad between the two correlations */
            _add_skin_panel(model,
                            prev_corr.t_env_i, curr_corr.t_env_i, t_env, t_neighbors_map,
                            prev_corr.n_env_i, curr_corr.n_env_i, n_env, n_neighbors_map);
        else if (t_count == 1) {                    /* tailwise triangles fan */
            int trias_count = n_count - 1;
            int t_env_i = prev_corr.t_env_i;
            for (int k = 0; k < trias_count; ++k) {
                int n_env_i1 = (prev_corr.n_env_i + k) % n_env->count;
                int n_env_i2 = (n_env_i1 + 1) % n_env->count;
                _add_skin_panel(model,
                                t_env_i,        -1, t_env, t_neighbors_map,
                                n_env_i1, n_env_i2, n_env, n_neighbors_map);
            }
        }
        else if (n_count == 1) {                    /* nosewise triangles fan */
            int trias_count = t_count - 1;
            int n_env_i = prev_corr.n_env_i;
            for (int k = 0; k < trias_count; ++k) {
                int t_env_i1 = (prev_corr.t_env_i + k) % t_env->count;
                int t_env_i2 = (t_env_i1 + 1) % t_env->count;
                _add_skin_panel(model,
                                t_env_i1, t_env_i2, t_env, t_neighbors_map,
                                n_env_i,        -1, n_env, n_neighbors_map);
            }
        }
        else                                        /* multiple vertices between correlations */
            _mesh_pass_2(model,
                         t_env, prev_corr.t_env_i, curr_corr.t_env_i, t_neighbors_map,
                         n_env, prev_corr.n_env_i, curr_corr.n_env_i, n_neighbors_map);

        prev_corr = curr_corr;
    }
}

/* Handles simplest case when all the points can be correlated directly, one-to-one. */
static void _mesh_pass_0(Model *model, int shape_subdivs,
                         MeshEnv *t_env, int *t_neighbors_map,
                         MeshEnv *n_env, int *n_neighbors_map) {
    int t_i1 = 0;
    int t_subdiv_i = t_env->points[t_i1].subdiv_i;

    int n_i1 = 0;
    for (; n_i1 < n_env->count; ++n_i1)
        if (t_subdiv_i == n_env->points[n_i1].subdiv_i)
            break;
    break_assert(n_i1 < n_env->count); /* could not correlate first tail point with anything on the nose envelope */

    int t_i2 = period_incr(t_i1, t_env->count);
    int n_i2 = period_incr(n_i1, n_env->count);

    for (int j = 0; j < shape_subdivs; ++j) {
        _add_skin_panel(model,
                        t_i1, t_i2, t_env, t_neighbors_map,
                        n_i1, n_i2, n_env, n_neighbors_map);
        t_i1 = t_i2;
        n_i1 = n_i2;
        t_i2 = period_incr(t_i2, t_env->count);
        n_i2 = period_incr(n_i2, n_env->count);
    }
}

/* Main meshing procedure. */
void mesh_between_two_sections(Model *model, int shape_subdivs,
                               MeshEnv *t_env, int *t_neighbors_map,
                               MeshEnv *n_env, int *n_neighbors_map) {

#if DRAW_CORRS
    _init_corr(model, t_env->x, n_env->x);
#endif

    int first_panel_i = model->panels_count;

    if (t_env->slices_count == 1 && t_env->count == shape_subdivs &&
        n_env->slices_count == 1 && n_env->count == shape_subdivs)  /* simple case when both sections contain only one shape */
        _mesh_pass_0(model, shape_subdivs,
                     t_env, t_neighbors_map,
                     n_env, n_neighbors_map);
    else                                                            /* more complicated case where we don't have clear correlations between all points */
        _mesh_pass_1(model, shape_subdivs,
                     t_env, t_neighbors_map,
                     n_env, n_neighbors_map);

    /* fix first and last panels neighbors */

    model->panels[first_panel_i].prev = model->panels_count - 1;
    model->panels[model->panels_count - 1].next = first_panel_i;
}
