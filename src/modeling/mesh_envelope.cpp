#include "mesh.h"
#include "../periodic.h"
#include "shape.h"
#include "model.h"
#include "config.h"
#include "../arena.h"
#include "../vec.h"
#include <stdlib.h>
#include <assert.h>


double COLLAPSE_MARGIN = 0.25;
double ONE_MINUS_COLLAPSE_MARGIN = 1.0 - COLLAPSE_MARGIN;

static void _update_mesh_envelope_slices(MeshEnv *env) {
    env->slices_count = 0;
    MeshEnvSlice *slice = 0;

    for (int i = 0; i < env->count; ++i) {
        MeshPoint *p = env->points + i;
        if (p->is_intersection)
            slice = 0; /* close slice */
        else {
            if (slice == 0) { /* start new slice */
                slice = env->slices + env->slices_count++;
                slice->beg = i;
                slice->ids = p->ids;
            }
            slice->end = i; /* continue slice */
        }
    }

    MeshEnvSlice *beg_slice = env->slices;
    MeshEnvSlice *end_slice = env->slices + env->slices_count - 1;
    if (env->slices_count > 1 && beg_slice->beg == 0 && end_slice->end == env->count - 1) { /* maybe merge first and last slices */
        beg_slice->beg = end_slice->beg;
        --env->slices_count;
    }
}

static void _add_mesh_point(MeshEnv *env, EnvPoint *ep, int i1, int i2, int vert_i) {
    MeshPoint *mp = env->points + env->count++;
    mp->x = ep->x;
    mp->y = ep->y;
    mp->i1 = i1;
    mp->i2 = i2;
    mp->ids = ep->ids;
    mp->vert_i = vert_i;
    mp->subdiv_i = ep->subdiv_i;
    mp->is_intersection = ep->is_intersection;
    mp->t_is_outermost = true;
    mp->n_is_outermost = true;
}

/* Check if current non-intersection point should be skipped. This could be due
to previous or next point being intersection. */
static inline bool _do_we_skip_non_intersection(bool prev_is_intersection, double prev_t2,
                                                bool next_is_intersection, double next_t1) {
    if (prev_is_intersection && prev_t2 > ONE_MINUS_COLLAPSE_MARGIN)
        return true;
    if (next_is_intersection && next_t1 < COLLAPSE_MARGIN)
        return true;
    return false;
}

/* Should intersection point get new i1 or i2 if corresponding non-intersection point was skipped. */
static inline void _do_we_fix_intersection_i1_i2(bool prev_is_intersection, int prev_i1,
                                                 bool next_is_intersection, int next_i2,
                                                 double t1, double t2, int *i1, int *i2) {
    if (!prev_is_intersection && t1 < COLLAPSE_MARGIN)
        *i1 = prev_i1;
    if (!next_is_intersection && t2 > ONE_MINUS_COLLAPSE_MARGIN)
        *i2 = next_i2;
}

/* Creates mesh envelope points for one side of an opening. */
static int _mesh_envs_pass_3(float section_x, vec3 *verts, int verts_count,
                             MeshEnv *env, EnvPoint *env_points, int count, int beg, int end,
                             double beg_t, double end_t) {

    int _beg = period_incr(beg, count);
    int _end = period_decr(end, count);

    for (int i = _beg; i != end; i = period_incr(i, count)) {
        EnvPoint *ep = env_points + i;
        int i1 = ep->i1;
        int i2 = ep->i2;
        bool skip = false;

        bool prev_is_intersection;
        bool next_is_intersection;
        int prev_i1, next_i2;
        double prev_t2, next_t1;

        if (i == _beg) {
            prev_is_intersection = true;
            prev_i1 = -1;
            prev_t2 = beg_t;
        }
        else {
            EnvPoint *prev_ep = env_points + period_decr(i, count);
            prev_is_intersection = prev_ep->is_intersection;
            prev_i1 = prev_ep->i1;
            prev_t2 = prev_ep->t2;
        }

        if (i == _end) {
            next_is_intersection = true;
            next_i2 = -1;
            next_t1 = end_t;
        }
        else {
            EnvPoint *next_ep = env_points + period_incr(i, count);
            next_is_intersection = next_ep->is_intersection;
            next_i2 = next_ep->i2;
            next_t1 = next_ep->t1;
        }

        if (ep->is_intersection)
            _do_we_fix_intersection_i1_i2(prev_is_intersection, prev_i1,
                                          next_is_intersection, next_i2,
                                          ep->t1, ep->t2, &i1, &i2);
        else
            skip = _do_we_skip_non_intersection(prev_is_intersection, prev_t2,
                                                next_is_intersection, next_t1);

        if (!skip) {
            _add_mesh_point(env, ep, i1, i2, verts_count);
            vec3 *v = verts + verts_count++;
            v->x = section_x;
            v->y = (float)ep->x;
            v->z = (float)ep->y;
        }
    }

    return verts_count;
}

enum _CorrType { ctNone, ctIsec, ctOpen };

static struct _Corr {
    _CorrType type;
    bool tail_isec_opening; /* if opening, intersections are on the tail side */
    union {
        int t_i;
        int t_beg_i; /* if opening */
    };
    union {
        int n_i;
        int n_beg_i; /* if opening */
    };
    int t_end_i; /* if opening */
    int n_end_i; /* if opening */
} *corrs = 0;

/* Creates mesh envelope points for both sides of an opening. */
static int _mesh_envs_pass_2(float section_x, vec3 *verts, int verts_count, _Corr *corr,
                             MeshEnv *t_env, EnvPoint *t_env_points, int t_count,
                             MeshEnv *n_env, EnvPoint *n_env_points, int n_count) {
    EnvPoint *ep1 = 0, *ep2 = 0;
    double t_beg_t, t_end_t;
    double n_beg_t, n_end_t;

    assert(corr->type == ctOpen); /* correlation must be opening */

    if (corr->tail_isec_opening) {
        ep1 = t_env_points + corr->t_beg_i;
        ep2 = t_env_points + corr->t_end_i;
        t_beg_t = ep1->t2;
        t_end_t = ep2->t1;
        n_beg_t = ep1->t1;
        n_end_t = ep2->t2;
    }
    else {
        ep1 = n_env_points + corr->n_beg_i;
        ep2 = n_env_points + corr->n_end_i;
        t_beg_t = ep1->t1;
        t_end_t = ep2->t2;
        n_beg_t = ep1->t2;
        n_end_t = ep2->t1;
    }

    /* clone first tail isec */

    _add_mesh_point(t_env, ep1, ep1->i1, ep1->i2, verts_count);
    _add_mesh_point(n_env, ep1, ep1->i1, ep1->i2, verts_count);

    vec3 *v = verts + verts_count++;
    v->x = section_x;
    v->y = (float)ep1->x;
    v->z = (float)ep1->y;

    /* add the rest of the points in between */

    verts_count = _mesh_envs_pass_3(section_x, verts, verts_count,
                                    t_env, t_env_points, t_count, corr->t_beg_i, corr->t_end_i,
                                    t_beg_t, t_end_t);

    verts_count = _mesh_envs_pass_3(section_x, verts, verts_count,
                                    n_env, n_env_points, n_count, corr->n_beg_i, corr->n_end_i,
                                    n_beg_t, n_end_t);

    /* clone last tail isec */

    _add_mesh_point(t_env, ep2, ep2->i1, ep2->i2, verts_count);
    _add_mesh_point(n_env, ep2, ep2->i1, ep2->i2, verts_count);

    v = verts + verts_count++;
    v->x = section_x;
    v->y = (float)ep2->x;
    v->z = (float)ep2->y;

    return verts_count;
}

static int _check_for_opening_corrs(Model *model, _Corr *corrs, int corrs_count,
                                    EnvPoint *t_env_points, int t_count, int t_beg, int t_end,
                                    EnvPoint *n_env_points, int n_count, int n_beg, int n_end) {

    int t_range = period_range_count(t_beg, t_end, t_count);
    int n_range = period_range_count(n_beg, n_end, n_count);

    if (t_range <= 3 && n_range <= 3) /* nothing to correlate */
        return corrs_count;

    bool t_has_isecs = false;
    int _t_beg = t_beg, _t_end = t_end;
    EnvPoint *t_ep1 = 0, *t_ep2 = 0;

    if (t_range > 3) {
        _t_beg = period_incr(t_beg, t_count);
        _t_end = period_decr(t_end, t_count);
        t_ep1 = t_env_points + _t_beg;
        t_ep2 = t_env_points + _t_end;

        if (t_ep1->is_intersection && t_ep2->is_intersection)
            t_has_isecs = true;
    }

    bool n_has_isecs = false;
    int _n_beg = n_beg, _n_end = n_end;
    EnvPoint *n_ep1 = 0, *n_ep2 = 0;

    if (n_range > 3) {
        _n_beg = period_incr(n_beg, n_count);
        _n_end = period_decr(n_end, n_count);
        n_ep1 = n_env_points + _n_beg;
        n_ep2 = n_env_points + _n_end;

        if (n_ep1->is_intersection && n_ep2->is_intersection)
            n_has_isecs = true;
    }

    /* decide which side to clone (if any) */

    if (t_has_isecs && n_has_isecs) {
        if (t_ep1->t1 < n_ep1->t1 && t_ep2->t2 > n_ep2->t2)
            n_has_isecs = false;
        else if (t_ep1->t1 > n_ep1->t1 && t_ep2->t2 < n_ep2->t2)
            t_has_isecs = false;
    }

    model_assert(model, t_has_isecs != n_has_isecs, "_check_for_opening_corrs"); /* could not decide whether opening is tail or nose */

    if (t_has_isecs) {
        _Corr *corr = corrs + corrs_count++;
        corr->t_beg_i = _t_beg;
        corr->t_end_i = _t_end;
        corr->n_beg_i = n_beg;
        corr->n_end_i = n_end;
        corr->type = ctOpen;
        corr->tail_isec_opening = true;
    }
    else {
        _Corr *corr = corrs + corrs_count++;
        corr->t_beg_i = t_beg;
        corr->t_end_i = t_end;
        corr->n_beg_i = _n_beg;
        corr->n_end_i = _n_end;
        corr->type = ctOpen;
        corr->tail_isec_opening = false;
    }

    return corrs_count;
}

static inline EnvPoint *_end_isec_point_from_corr(_Corr *corr, EnvPoint *t_env_points, EnvPoint *n_env_points) {
    if (corr->type == ctOpen) {
        if (corr->tail_isec_opening)
            return t_env_points + corr->t_end_i;
        else
            return n_env_points + corr->n_end_i;
    }
    return t_env_points + corr->t_i;
}

static inline EnvPoint *_beg_isec_point_from_corr(_Corr *corr, EnvPoint *t_env_points, EnvPoint *n_env_points) {
    if (corr->type == ctOpen) {
        if (corr->tail_isec_opening)
            return t_env_points + corr->t_beg_i;
        else
            return n_env_points + corr->n_beg_i;
    }
    return t_env_points + corr->t_i;
}

static bool _can_correlate_env_points(EnvPoint *a, EnvPoint *b) {
    if (a->is_intersection != b->is_intersection ||
        a->ids.tail != b->ids.tail ||
        a->ids.nose != b->ids.nose ||
        a->subdiv_i != b->subdiv_i)
        return false;
    if (!a->is_intersection) /* if non-intersection, we're done */
        return true;
    double dx = a->x - b->x;
    if (dx > 0.000001 || dx < -0.000001)
        return false;
    double dy = a->y - b->y;
    if (dy > 0.000001 || dy < -0.000001)
        return false;
    return true;
}

/* Creates mesh points from two envelopes. */
static void _mesh_envs_pass_1(Model *model, Arena *verts_arena, float section_x,
                              MeshEnv *t_env, TraceEnv *t_trace_env,
                              MeshEnv *n_env, TraceEnv *n_trace_env) {

    if (corrs == 0)
        corrs = (_Corr *)malloc(sizeof(_Corr) * MAX_ENVELOPE_POINTS);
    int corrs_count = 0;

    t_env->count = 0;
    n_env->count = 0;
    t_env->verts_base_i = model->skin_verts_count;
    n_env->verts_base_i = model->skin_verts_count;
    t_env->object_like_flags = t_trace_env->object_like_flags;
    n_env->object_like_flags = n_trace_env->object_like_flags;

    vec3 *verts = verts_arena->rest<vec3>();
    int verts_count = 0;

    /* collect all direct and opening correlations */

    EnvPoint *t_env_points = t_trace_env->points;
    EnvPoint *n_env_points = n_trace_env->points;
    int t_count = t_trace_env->count;
    int n_count = n_trace_env->count;

    _Corr *first_corr = 0;
    _Corr *prev_corr = 0;

    for (int t_i = 0; t_i < t_count; ++t_i) {
        EnvPoint *t_ep = t_env_points + t_i;

        for (int n_i = 0; n_i < n_count; ++n_i) { // TODO: optimize this loop, don't loop through already correlated points
            EnvPoint *n_ep = n_env_points + n_i;

            if (_can_correlate_env_points(t_ep, n_ep)) {

                if (prev_corr) /* check for zip correlations */
                    corrs_count = _check_for_opening_corrs(model, corrs, corrs_count,
                                                           t_env_points, t_count, prev_corr->t_i, t_i,
                                                           n_env_points, n_count, prev_corr->n_i, n_i);

                _Corr *corr = corrs + corrs_count++;
                corr->t_i = t_i;
                corr->n_i = n_i;
                corr->type = t_ep->is_intersection ? ctIsec : ctNone;

                if (first_corr == 0)
                    first_corr = corr;
                prev_corr = corr;
            }
        }
    }

    corrs_count = _check_for_opening_corrs(model, corrs, corrs_count,
                                           t_env_points, t_count, prev_corr->t_i, first_corr->t_i,
                                           n_env_points, n_count, prev_corr->n_i, first_corr->n_i);

    /* use correlations to create mesh envelopes */

    for (int i = 0; i < corrs_count; ++i) {
        _Corr *corr = corrs + i;

        if (corr->type != ctOpen) { /* non-opening correlation */
            EnvPoint *t_ep = t_env_points + corr->t_i;
            EnvPoint *n_ep = n_env_points + corr->n_i;
            int i1 = t_ep->i1;
            int i2 = t_ep->i2;
            bool skip = false;

            /* check for collapse */

            _Corr *prev_corr = corrs + period_decr(i, corrs_count);
            _Corr *next_corr = corrs + period_incr(i, corrs_count);

            if (corr->type == ctNone) {
                EnvPoint *prev_ep = _end_isec_point_from_corr(prev_corr, t_env_points, n_env_points);
                EnvPoint *next_ep = _beg_isec_point_from_corr(next_corr, t_env_points, n_env_points);

                skip = _do_we_skip_non_intersection(prev_corr->type != ctNone, prev_ep->t2,
                                                    next_corr->type != ctNone, next_ep->t1);
            }
            else {
                EnvPoint *beg_ep = _beg_isec_point_from_corr(corr, t_env_points, n_env_points);
                EnvPoint *end_ep = _end_isec_point_from_corr(corr, t_env_points, n_env_points);

                _do_we_fix_intersection_i1_i2(prev_corr->type != ctNone, (t_env_points + prev_corr->t_i)->i1,
                                              next_corr->type != ctNone, (t_env_points + next_corr->t_i)->i2,
                                              beg_ep->t1, end_ep->t2, &i1, &i2);
            }

            /* add to both mesh envelopes */

            if (!skip) {
                _add_mesh_point(t_env, t_ep, i1, i2, verts_count);
                _add_mesh_point(n_env, n_ep, i1, i2, verts_count);

                vec3 *v = verts + verts_count++;
                v->x = section_x;
                v->y = (float)t_ep->x;
                v->z = (float)t_ep->y;
            }
        }
        else
            verts_count = _mesh_envs_pass_2(section_x, verts, verts_count, corr,
                                            t_env, t_env_points, t_count,
                                            n_env, n_env_points, n_count);
    }

    model->skin_verts_count += verts_count;
    verts_arena->alloc<vec3>(verts_count);

    _update_mesh_envelope_slices(t_env);
    _update_mesh_envelope_slices(n_env);
}

/* Handles simple case when there's only one shape in envelope. */
void _mesh_envs_pass_0(Model *model, Arena *verts_arena, float section_x,
                       MeshEnv *env, TraceEnv *trace_env) {

    /* make mesh envelope from trace envelope */

    env->count = 0;
    env->verts_base_i = model->skin_verts_count;
    env->object_like_flags = trace_env->object_like_flags;

    vec3 *verts = verts_arena->rest<vec3>();
    int verts_count = 0;

    for (int i = 0; i < trace_env->count; ++i) {
        EnvPoint *ep = trace_env->points + i;
        EnvPoint *prev_ep = trace_env->points + ((i == 0) ? (trace_env->count - 1) : (i - 1));
        EnvPoint *next_ep = trace_env->points + ((i == (trace_env->count - 1)) ? 0 : (i + 1));
        int i1 = ep->i1;
        int i2 = ep->i2;
        bool skip = false;

        /* tests for collapsing points */

        if (!ep->is_intersection)
            skip = _do_we_skip_non_intersection(prev_ep->is_intersection, prev_ep->t2,
                                                next_ep->is_intersection, next_ep->t1);
        else
            _do_we_fix_intersection_i1_i2(prev_ep->is_intersection, prev_ep->i1,
                                          next_ep->is_intersection, next_ep->i2,
                                          ep->t1, ep->t2, &i1, &i2);

        /* add new mesh point and copy data from envelope point */

        if (!skip) {
            _add_mesh_point(env, ep, i1, i2, env->count);

            vec3 *v = verts + verts_count++;
            v->x = section_x;
            v->y = (float)ep->x;
            v->z = (float)ep->y;
        }
    }

    model->skin_verts_count += verts_count;
    verts_arena->alloc<vec3>(verts_count);

    /* group non-intersection envelope points into slices */

    _update_mesh_envelope_slices(env);
}

/* Main mesh envelope making procedure. */
void mesh_make_envelopes(Model *model, Arena *verts_arena, float section_x,
                         MeshEnv *t_env, TraceEnv *t_trace_env,
                         MeshEnv *n_env, TraceEnv *n_trace_env) {

    t_env->x = n_env->x = section_x;

    if (t_env == n_env) /* single envelope */
        _mesh_envs_pass_0(model, verts_arena, section_x,
                          t_env, t_trace_env);
    else                /* double envelope */
        _mesh_envs_pass_1(model, verts_arena, section_x,
                          t_env, t_trace_env,
                          n_env, n_trace_env);
}

void mesh_verts_merge_margin(bool increase) {
    if (increase) {
        if (COLLAPSE_MARGIN < 0.5) {
            COLLAPSE_MARGIN += 0.05;
            ONE_MINUS_COLLAPSE_MARGIN = 1.0 - COLLAPSE_MARGIN;
        }
    }
    else {
        if (COLLAPSE_MARGIN > 0.0) {
            COLLAPSE_MARGIN -= 0.05;
            ONE_MINUS_COLLAPSE_MARGIN = 1.0 - COLLAPSE_MARGIN;
        }
    }
}
