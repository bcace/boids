#include "mesh.h"
#include "shape.h"
#include "arena.h"
#include "dvec.h"
#include "debug.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#define MAX_TRIES               100
#define RETRY_STEP              1.0e-10
#define RETRY_MARGIN            1.0e-15
#define ONE_PLUS_RETRY_MARGIN   (1.0 + RETRY_MARGIN)
#define BROAD_PHASE_MARGIN      (RETRY_STEP * MAX_TRIES)
#define PARALLEL_MARGIN         1.0e-10 /* updates, old to new: 1.0e-5 */
#define MIN_WEIGHT_RATIO        0.9
#define MAX_WEIGHT_RATIO        (1.0 / MIN_WEIGHT_RATIO)


static Arena env_arena(200000);

/* BUNDLE_MARGIN_FACTOR should be:
- larger to avoid dithering between shapes being in or out of a bundle, because farther from the merged object
connections are converging faster
- smaller to avoid large differences between envelopes with and without bundles
- smaller to avoid large differences between normals at samples, which make deciding which sample to take more uncertain
- smaller to avoid jumps in fill polygons, because they're formed from shape bundle polygon centers */
const double BUNDLE_MARGIN_FACTOR = 0.05;

/* Calculates polygon x offset. Offset is non-zero only when try_i is non-zero. */
static inline double _try_offset_x(int shape_i, int try_i, double angle_step) {
    return cos(shape_i * angle_step) * try_i * RETRY_STEP;
}

/* Calculates polygon y offset. Offset is non-zero only when try_i is non-zero. */
static inline double _try_offset_y(int shape_i, int try_i, double angle_step) {
    return sin(shape_i * angle_step) * try_i * RETRY_STEP;
}

struct _Bounds {
    dvec min, max;
};

struct _Poly {
    Shape *shapes[MAX_ENVELOPE_SHAPES];
    int shapes_count;
    dvec *verts;
    Origin origin;
    int verts_count;
    _Bounds bounds[SHAPE_CURVES];
    dvec center;
};

static inline void _init_poly(_Poly *poly, OriginPart t_origin, OriginPart n_origin) {
    poly->shapes_count = 0;
    poly->origin.tail = t_origin;
    poly->origin.nose = n_origin;
    for (int i = 0; i < SHAPE_CURVES; ++i) {
        _Bounds *bounds = poly->bounds + i;
        bounds->min.x = DBL_MAX;
        bounds->min.y = DBL_MAX;
        bounds->max.x = -DBL_MAX;
        bounds->max.y = -DBL_MAX;
    }
}

static inline bool _should_bundle_shapes(Shape *a, Shape *b, double mx, double my) {
    for (int i = 0; i < SHAPE_CURVES; ++i) {

        /* Check how close curves' start points are: distances between corresponding vertices
        (in both directions) cannot be larger than respective margin. */

        double dx = a->curves[i].x - b->curves[i].x;
        if (dx < -mx || dx > mx)
            return false;
        double dy = a->curves[i].y - b->curves[i].y;
        if (dy < -my || dy > my)
            return false;

        /* Check how different curve weights are: ratio between corresponding curve weights cannot
        be outside [MIN_WEIGHT_RATIO, MAX_WEIGHT_RATIO]. */

        double dw = a->curves[i].w / b->curves[i].w;
        if (dw < MIN_WEIGHT_RATIO || dw > MAX_WEIGHT_RATIO)
            return false;
    }

    return true;
}

/* Main envelope tracing function. TODO: describe arguments. */
int mesh_trace_envelope(EnvPoint *env_points, Shape **shapes, int shapes_count, int curve_subdivs, OriginFlag *object_like_flags) {
    break_assert(shapes_count < MAX_ENVELOPE_SHAPES);
    break_assert(curve_subdivs >= MIN_CURVE_SUBDIVS);
    break_assert(curve_subdivs <= MAX_CURVE_SUBDIVS);

    env_arena.clear();

    if (shapes_count == 0) /* no shapes, no envelope */
        return 0;

    int env_count = 0;
    int shape_subdivs = SHAPE_CURVES * curve_subdivs;

    /* Bundle shapes into polygons. Most of the polygons will only contain a single shape, but in
    merge situations we want those shapes that almost perfectly overlap each other to be represented
    by a single polygon when tracing. */

    _Poly polys[MAX_ENVELOPE_SHAPES + 1];
    int polys_count = 0;

    {
        /* Shape bundling:
        - Consider only conn shapes.
        - Compare only conn shapes that belong to the same merge group.
        - Assign bundling decision to object origin part.
        - Another pass that says:
            - If object shape - single shape poly.
            - If conn shape whose neither origin parts are bundled - single shape poly.
            - Else, add shape to appropriate bundle poly. */

        _Poly *bundle_polys_map[MAX_FUSELAGE_OBJECTS];
        memset(bundle_polys_map, 0, sizeof(_Poly *) * MAX_FUSELAGE_OBJECTS);

        for (int a_i = 0; a_i < shapes_count; ++a_i) {
            Shape *a = shapes[a_i];
            if (a->origin.tail == a->origin.nose) /* conn shapes only */
                continue;

            double a_dx = a->curves[0].x - a->curves[2].x;
            double a_dy = a->curves[1].y - a->curves[3].y;

            for (int b_i = a_i + 1; b_i < shapes_count; ++b_i) {
                Shape *b = shapes[b_i];
                if (b->origin.tail == b->origin.nose) /* conn shapes only */
                    continue;

                bool t_merge = a->origin.tail == b->origin.tail;
                bool n_merge = a->origin.nose == b->origin.nose;

                if (t_merge == n_merge) /* must belong to same merge group */
                    continue;

                double b_dx = b->curves[0].x - b->curves[2].x;
                double b_dy = b->curves[1].y - b->curves[3].y;
                double mx = ((a_dx > b_dx) ? a_dx : b_dx) * BUNDLE_MARGIN_FACTOR;
                double my = ((a_dy > b_dy) ? a_dy : b_dy) * BUNDLE_MARGIN_FACTOR;

                if (_should_bundle_shapes(a, b, mx, my)) {
                    OriginPart o_origin = t_merge ? a->origin.tail : a->origin.nose;
                    if (bundle_polys_map[o_origin] == 0) {
                        _Poly *p = polys + polys_count++;
                        bundle_polys_map[o_origin] = p;
                        _init_poly(p, o_origin, o_origin);
                    }
                }
            }
        }

        for (int i = 0; i < shapes_count; ++i) {
            Shape *s = shapes[i];

            if (s->origin.tail == s->origin.nose) { /* object shape */
                _Poly *p = polys + polys_count++;
                _init_poly(p, s->origin.tail, s->origin.nose);
                p->shapes[p->shapes_count++] = s;
            }
            else { /* conn shape */
                _Poly *t_bundle_p = bundle_polys_map[s->origin.tail];
                _Poly *n_bundle_p = bundle_polys_map[s->origin.nose];

                break_assert(t_bundle_p == 0 || n_bundle_p == 0); /* shape sohuld not be in two bundles at the same time */

                if (t_bundle_p)
                    t_bundle_p->shapes[t_bundle_p->shapes_count++] = s;
                else if (n_bundle_p)
                    n_bundle_p->shapes[n_bundle_p->shapes_count++] = s;
                else {
                    _Poly *p = polys + polys_count++;
                    _init_poly(p, s->origin.tail, s->origin.nose);
                    p->shapes[p->shapes_count++] = s;
                }
            }
        }
    }

    /* collect all object-like polygons */

    *object_like_flags = 0ull;

    for (int i = 0; i < polys_count; ++i) {
        _Poly *p = polys + i;
        if (p->origin.tail == p->origin.nose)
            *object_like_flags |= origin_index_to_flag(p->origin.tail);
    }

    /* sample polygons */

    {
        double dt = 1.0 / curve_subdivs;

        for (int i = 0; i < polys_count; ++i) {
            _Poly *p = polys + i;
            p->verts = env_arena.alloc<dvec>(shape_subdivs);
            p->verts_count = shape_subdivs;

            if (p->shapes_count == 1) { /* simple case when there's only one shape in polygon */
                Curve *curves = p->shapes[0]->curves;

                for (int j = 0; j < SHAPE_CURVES; ++j) {
                    Curve *curve1 = curves + j;
                    Curve *curve2 = curves + ((j + 1) % SHAPE_CURVES);
                    dvec *curve_verts = p->verts + j * curve_subdivs;

                    for (int k = 0; k < curve_subdivs; ++k)
                        curve_verts[k] = shape_bezier(
                            curve1->x, curve1->y,
                            curve1->cx, curve1->cy,
                            curve2->x, curve2->y,
                            k * dt, curve1->w);
                }

                p->center = shape_centroid(p->shapes[0]);
            }
            else {

                /* In this case there are multiple shapes in the polygon. We first sample vertices for all shapes,
                putting all vertices for a subdivision next to each other. Then we find the outermost one of these
                along the average normal. */

                dvec *verts = env_arena.lock<dvec>(shape_subdivs * p->shapes_count);
                dvec centroid = mesh_polygonize_shape_bundle(p->shapes, p->shapes_count, shape_subdivs, verts);

                static int outermost_shape_indices[MAX_FUSELAGE_OBJECTS];
                double subdiv_da = TAU / shape_subdivs;

                for (int subdiv_i = 0; subdiv_i < shape_subdivs; ++subdiv_i) { /* find outermost vertex for each subdivision */
                    int count = mesh_find_outermost_shapes_for_subdivision(verts, centroid, subdiv_i, subdiv_da, p->shapes_count, outermost_shape_indices);
                    dvec *subdiv_verts = verts + subdiv_i * p->shapes_count;

                    if (count == 1)     /* single outermost vertex found */
                        p->verts[subdiv_i] = subdiv_verts[outermost_shape_indices[0]];
                    else {              /* multiple outermost vertices found */
                        dvec r;
                        r.x = r.y = 0.0;
                        for (int c = 0; c < count; ++c) {
                            dvec v = subdiv_verts[outermost_shape_indices[c]];
                            r.x += v.x;
                            r.y += v.y;
                        }
                        r.x /= count;
                        r.y /= count;

                        p->verts[subdiv_i] = r;
                    }
                }

                env_arena.unlock();

                p->center.x = 0.0;
                p->center.y = 0.0;
                for (int j = 0; j < p->shapes_count; ++j) { /* calculate bundle centroid */
                    dvec c = shape_centroid(p->shapes[j]);
                    p->center.x += c.x;
                    p->center.y += c.y;
                }
                p->center.x /= p->shapes_count;
                p->center.y /= p->shapes_count;
            }

            for (int j = 0; j < SHAPE_CURVES; ++j) { /* update bundle quadrant bounds */
                dvec *quad_min = &p->bounds[j].min;
                dvec *quad_max = &p->bounds[j].max;

                for (int k = 0; k <= curve_subdivs; ++k) {
                    int subdiv_i = (j * curve_subdivs + k) % shape_subdivs;
                    dvec v = p->verts[subdiv_i];
                    if (v.x < quad_min->x)
                        quad_min->x = v.x;
                    if (v.y < quad_min->y)
                        quad_min->y = v.y;
                    if (v.x > quad_max->x)
                        quad_max->x = v.x;
                    if (v.y > quad_max->y)
                        quad_max->y = v.y;
                }

                quad_min->x -= BROAD_PHASE_MARGIN;
                quad_min->y -= BROAD_PHASE_MARGIN;
                quad_max->x += BROAD_PHASE_MARGIN;
                quad_max->y += BROAD_PHASE_MARGIN;
            }
        }
    }

    /* fill polygon */

    {
        dvec centers[MAX_ENVELOPE_SHAPES];
        bool ignored[MAX_ENVELOPE_SHAPES];

        for (int i = 0; i < polys_count; ++i) { /* prepare all the shapes' centers */
            centers[i] = polys[i].center;
            ignored[i] = false;
        }

        int first_center = -1;

        double min_x = DBL_MAX;
        for (int i = 0; i < polys_count; ++i) { /* find the starting shape center */
            if (centers[i].x < min_x) {
                min_x = centers[i].x;
                first_center = i;
            }
        }

        /* trace fill polygon */

        _Poly *poly = polys + polys_count; /* take the first available poly, but don't make it yet */
        poly->verts = env_arena.lock<dvec>(shapes_count);
        poly->verts_count = 0;
        _init_poly(poly, -1, -1);

        {
            int current_center = first_center;
            double sx = 0.0;
            double sy = -1.0;

            poly->verts[poly->verts_count++] = centers[first_center];
            ignored[first_center] = true;

            int fill_guard = 0;
            for (; fill_guard < MAX_ENVELOPE_SHAPES; ++fill_guard) {
                int min_i = -1;
                double min_angle = DBL_MAX;
                double min_sx, min_sy;

                for (int i = 0; i < polys_count; ++i) {
                    if (i == current_center || ignored[i])
                        continue;
                    double dx = centers[i].x - centers[current_center].x;
                    double dy = centers[i].y - centers[current_center].y;
                    double dl = sqrt(dx * dx + dy * dy);
                    if (dl < 0.00001) { /* skip point if it is too close */
                        ignored[i] = true;
                        continue;
                    }
                    dx /= dl;
                    dy /= dl;
                    if (signbit(sx * dy - sy * dx)) /* if angle is negative */
                        continue;
                    double angle = acos(sx * dx + sy * dy);
                    if (angle < min_angle) {
                        min_angle = angle;
                        min_i = i;
                        min_sx = dx;
                        min_sy = dy;
                    }
                }

                if (min_i == -1 || min_i == first_center) /* no more acceptable new centers found, finish tracing */
                    break;
                else {
                    current_center = min_i;
                    sx = min_sx;
                    sy = min_sy;
                    poly->verts[poly->verts_count++] = centers[current_center];
                    ignored[min_i] = true;
                }
            }

            if (fill_guard == MAX_ENVELOPE_SHAPES) /* tracing fill polygon failed */
                return -1;
        }

        env_arena.unlock();

        /* if there is a fill polygon allocate vertices that have just been unlocked and add the bundle to the others */

        if (poly->verts_count > 1) {
            env_arena.alloc<dvec>(poly->verts_count);
            ++polys_count;
        }
    }

    /* Trace envelope. This algorithm takes prepared polygons and traces their outline or envelope.

    BASIC ALGORITHM:
        Find the trace starting point by going through all polygons and finding the most extreme vertex. Starting point
    is identified as polygon index and point index on that polygon.
        Continue tracing by testing whether the current polygon's side intersects any other polygon side (in the right
    direction). If there is an intersection with an outward going side of some other polygon, we add this intersection
    to envelope, otherwise we move to the next point of the same polygon and add it to envelope.
        Outward going side means that we only consider intersecting sides that will guide the trace from within one
    polygon out.

    TRIES:
        Basic tracing algorithm described above only uses line segment intersection so it's fairly precise, but still
    can produce errors which can guide the trace in a completely wrong direction. To avoid these situations whenever
    there's a numeric uncertainty we move all the polygons in different directions by some small distance and try again.
        There are two sources of numeric uncertainty: when a calculated intersection is too close to a polygon point,
    and when two tested polygon sides are parallel and too close to each other. */

    int try_i = 0;
    double try_angle_step = 6.28318530718 / polys_count;

    for (; try_i < MAX_TRIES; ++try_i) {

        /* find first envelope point */

        EnvPoint beg_point;
        beg_point.is_intersection = false;
        beg_point.i1 = -1;
        beg_point.i2 = -1;
        beg_point.subdiv_i = -1;
        beg_point.t1 = 0.0;
        beg_point.t2 = 0.0;
        int beg_point_poly_i = -1;

        {
            double beg_min_x = DBL_MAX; // TODO: maybe start with most extreme of the curve points

            for (int i = 0; i < polys_count; ++i) {
                _Poly *poly = polys + i;
                if (poly->shapes_count == 0) /* skip fill polygon */
                    continue;
                double offset_x = _try_offset_x(i, try_i, try_angle_step);
                double offset_y = _try_offset_y(i, try_i, try_angle_step);

                for (int j = curve_subdivs; j < curve_subdivs * 3; ++j) {
                    dvec p = poly->verts[j];
                    p.x += offset_x;
                    p.y += offset_y;

                    if (p.x < beg_min_x) {
                        beg_point.x = p.x;
                        beg_point.y = p.y;
                        beg_point_poly_i = i;
                        beg_point.i1 = (j == 0) ? (shape_subdivs - 1) : (j - 1);
                        beg_point.i2 = j;
                        beg_point.subdiv_i = j;
                        beg_point.origin = poly->origin;
                        beg_min_x = p.x;
                    }
                }
            }
        }

        /* trace envelope */

        EnvPoint point = beg_point;
        env_count = 0;
        env_points[env_count++] = point;
        int point_poly_i = beg_point_poly_i;

        int env_guard = 0;
        for (; env_guard < MAX_ENVELOPE_POINTS; ++env_guard) {

            bool is_fill_poly = polys[point_poly_i].shapes_count == 0;
            double try_offset_x = _try_offset_x(point_poly_i, try_i, try_angle_step);
            double try_offset_y = _try_offset_y(point_poly_i, try_i, try_angle_step);
            int poly_verts_count = polys[point_poly_i].verts_count;

            dvec s1, s2;
            s1 = polys[point_poly_i].verts[point.subdiv_i];
            s2 = polys[point_poly_i].verts[(point.subdiv_i + 1) % poly_verts_count];

            if (is_fill_poly) { /* inflate fill polygon sides */
                double sx = s2.x - s1.x;
                double sy = s2.y - s1.y;
                double sl = sqrt(sx * sx + sy * sy); // FAST_SQRT
                double nx = sy * SHAPE_FILL_POLY_MARGIN / sl;
                double ny = -sx * SHAPE_FILL_POLY_MARGIN / sl;
                s1.x += nx;
                s1.y += ny;
                s2.x += nx;
                s2.y += ny;
            }

            if (try_i != 0) { /* add try offset */
                s1.x += try_offset_x;
                s1.y += try_offset_y;
                s2.x += try_offset_x;
                s2.y += try_offset_y;
            }

            double s_min_x, s_min_y;
            double s_max_x, s_max_y;
            if (s1.x <= s2.x) { /* cache s1-s2 side extents */
                s_min_x = s1.x;
                s_max_x = s2.x;
            }
            else {
                s_min_x = s2.x;
                s_max_x = s1.x;
            }
            if (s1.y <= s2.y) {
                s_min_y = s1.y;
                s_max_y = s2.y;
            }
            else {
                s_min_y = s2.y;
                s_max_y = s1.y;
            }

            /* test other polygon' edges for intersection with s1-s2 edge */

            int min_poly_i = -1;
            int min_subdiv_i = -1;
            double min_t1 = 1.0;
            double min_t2 = 1.0;
            dvec min_p;

            for (int i = 0; i < polys_count; ++i) {
                if (i == point_poly_i) /* skip current edge's polygon */
                    continue;
                _Poly *other_poly = polys + i;

                bool other_is_fill_poly = other_poly->shapes_count == 0;
                double other_try_offset_x = _try_offset_x(i, try_i, try_angle_step);
                double other_try_offset_y = _try_offset_y(i, try_i, try_angle_step);
                int other_poly_verts_count = other_poly->verts_count;

                int broad_count = other_is_fill_poly ? 1 : SHAPE_CURVES;
                for (int j = 0; j < broad_count; ++j) { /* broad-phase: test if s1-s2 edge's bounds overlap with current shape's quadrant bounds */

                    _Bounds *bounds = other_poly->bounds + j;
                    if (!other_is_fill_poly &&
                        (s_min_x > bounds->max.x || s_max_x < bounds->min.x ||
                         s_min_y > bounds->max.y || s_max_y < bounds->min.y))
                        continue;

                    int narrow_count = other_is_fill_poly ? other_poly_verts_count : curve_subdivs;
                    for (int k = 0; k < narrow_count; ++k) { /* narrow-phase: test s1-s2 s3-s4 intersection */
                        int l = j * narrow_count + k;

                        dvec s3 = other_poly->verts[l];
                        dvec s4 = other_poly->verts[(l + 1 < other_poly_verts_count) ? l + 1 : 0];

                        if (other_is_fill_poly) { /* inflate fill polygon sides */
                            double sx = s4.x - s3.x;
                            double sy = s4.y - s3.y;
                            double sl = sqrt(sx * sx + sy * sy);
                            double nx = sy * SHAPE_FILL_POLY_MARGIN / sl;
                            double ny = -sx * SHAPE_FILL_POLY_MARGIN / sl;
                            s3.x += nx;
                            s3.y += ny;
                            s4.x += nx;
                            s4.y += ny;
                        }

                        if (try_i != 0) { /* add try offset */
                            s3.x += other_try_offset_x;
                            s3.y += other_try_offset_y;
                            s4.x += other_try_offset_x;
                            s4.y += other_try_offset_y;
                        }

                        double t1, t2;

                        { /* try intersecting lines */
                            double d = (s1.x - s2.x) * (s3.y - s4.y) - (s1.y - s2.y) * (s3.x - s4.x);
                            if (d > PARALLEL_MARGIN) /* wrong direction */
                                continue;
                            else if (d > -PARALLEL_MARGIN) { /* parallel lines */
                                double nx = s2.y - s1.y;
                                double ny = s1.x - s2.x;
                                double nl = sqrt(nx * nx + ny * ny);
                                nx /= nl;
                                ny /= nl;
                                double dx = s3.x - s1.x;
                                double dy = s3.y - s1.y;
                                double dot = nx * dx + ny * dy;
                                if (dot < RETRY_MARGIN && dot > -RETRY_MARGIN) /* if parallel edge lines are too close */
                                    goto RETRY_TRACE;
                                continue;
                            }
                            else { /* test for intersection */
                                t1 = ((s1.x - s3.x) * (s3.y - s4.y) - (s1.y - s3.y) * (s3.x - s4.x)) / d;
                                t2 = -((s1.x - s2.x) * (s1.y - s3.y) - (s1.y - s2.y) * (s1.x - s3.x)) / d;
                            }
                        }

                        /* permissive test if line segments intersect, because some retry tests below don't make sense if this doesn't pass */

                        if (t1 < -RETRY_MARGIN || t1 > ONE_PLUS_RETRY_MARGIN ||
                            t2 < -RETRY_MARGIN || t2 > ONE_PLUS_RETRY_MARGIN)
                            continue;

                        /* retry trace if intersection is too close to a polygon corner */

                        {
                            double d;
                            d = t1 - point.t2;
                            if (d < RETRY_MARGIN && d > -RETRY_MARGIN)
                                goto RETRY_TRACE;
                            d = t1 - min_t1;
                            if (d < RETRY_MARGIN && d > -RETRY_MARGIN)
                                goto RETRY_TRACE;
                            d = t2 - 0.0;
                            if (d < RETRY_MARGIN && d > -RETRY_MARGIN)
                                goto RETRY_TRACE;
                            d = t2 - 1.0;
                            if (d < RETRY_MARGIN && d > -RETRY_MARGIN)
                                goto RETRY_TRACE;
                        }

                        /* final test if line segments intersect */

                        if (t1 < point.t2 || t1 > min_t1 || t2 < 0.0 || t2 > 1.0)
                            continue;

                        min_t1 = t1;
                        min_t2 = t2;
                        min_poly_i = i;
                        min_subdiv_i = l;
                        min_p.x = s3.x + (s4.x - s3.x) * t2;
                        min_p.y = s3.y + (s4.y - s3.y) * t2;
                    }
                }
            }

            if (min_poly_i == -1) {     /* no intersection found */
                point.is_intersection = false;
                point.i1 = point.subdiv_i;
                int i2 = (point.subdiv_i + 1) % poly_verts_count;
                point.i2 = i2;
                point.subdiv_i = i2;
                point.t1 = 0.0;
                point.t2 = 0.0;
                point.x = polys[point_poly_i].verts[point.subdiv_i].x;
                point.y = polys[point_poly_i].verts[point.subdiv_i].y;
                point.origin = polys[point_poly_i].origin;
            }
            else {                      /* intersection found */
                point.is_intersection = true;
                point_poly_i = min_poly_i;
                point.i1 = point.subdiv_i;
                point.i2 = min_subdiv_i;
                point.subdiv_i = min_subdiv_i;
                point.t1 = min_t1;
                point.t2 = min_t2;
                point.x = min_p.x;
                point.y = min_p.y;
                point.origin = polys[min_poly_i].origin;
            }

            if (!point.is_intersection &&
                point_poly_i == beg_point_poly_i &&
                point.subdiv_i == beg_point.subdiv_i) { /* stop if start point reached */
                break;
            }
            else /* add points to envelope */
                env_points[env_count++] = point;
        }

        if (env_guard >= MAX_ENVELOPE_POINTS) /* max envelope points exceeded */
            return -1;

        break; /* get out of the try loop */

    RETRY_TRACE:;
    }

    if (try_i == MAX_TRIES) /* max retries exceeded */
        return -1;

    return env_count;
}
