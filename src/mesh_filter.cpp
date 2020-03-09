#include "mesh.h"
#include "shape.h"
#include "arena.h"
#include "dvec.h"
#include "periodic.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* For all the shapes' points in a subdivision only the ones within the _OUTERMOST_MARGIN
distance from the outermost point (in metres) will be considered as outermost as well. */
#define _OUTERMOST_MARGIN 1.0e-6

/* For an object-like origin provides shapes of connections connected to that object or bundle.
All shapes are located on two adjacent fuselage sections. */
struct _ConnsForObject {
    Shape *shapes[MAX_ELEM_REFS];
    int count;
};

/* If any merge transitions are detected between two given mesh envelopes marks appropriate mesh
points as non-outermost. */
void mesh_apply_merge_filter(Arena *arena, int shape_subdivs,
                             Shape **t_shapes, int t_shapes_count, MeshEnv *t_env,
                             Shape **n_shapes, int n_shapes_count, MeshEnv *n_env) {

    _ConnsForObject *conns = arena->lock<_ConnsForObject>(MAX_ELEM_REFS);
    dvec *verts = arena->lock<dvec>(MAX_SHAPE_SUBDIVS * MAX_ELEM_REFS);
    Flags *filter = arena->lock<Flags>(MAX_SHAPE_SUBDIVS);

    /* collect all connection shapes on one side for each object-like shape on the other */

    memset(conns, 0, sizeof(_ConnsForObject) * MAX_ELEM_REFS);

    for (int i = 0; i < t_shapes_count; ++i) {
        Shape *s = t_shapes[i];
        if (s->ids.tail != s->ids.nose && flags_contains(&n_env->object_like_flags, s->ids.nose)) {
            _ConnsForObject *c = conns + s->ids.nose;
            c->shapes[c->count++] = s;
        }
    }

    for (int i = 0; i < n_shapes_count; ++i) {
        Shape *s = n_shapes[i];
        if (s->ids.tail != s->ids.nose && flags_contains(&t_env->object_like_flags, s->ids.tail)) {
            _ConnsForObject *c = conns + s->ids.tail;
            c->shapes[c->count++] = s;
        }
    }

    /* for each object-like shape that transitions into multiple connections form and apply filter */

    double subdiv_da = TAU / shape_subdivs;

    for (int object_like_i = 0; object_like_i < MAX_ELEM_REFS; ++object_like_i) {
        _ConnsForObject *c = conns + object_like_i;

        Flags object_like_flag = flags_make(object_like_i);
        bool t_is_object_like = flags_and(&t_env->object_like_flags, &object_like_flag);
        bool n_is_object_like = flags_and(&n_env->object_like_flags, &object_like_flag);

        /* if object-like on one side and multiple connections on other side */

        if (t_is_object_like != n_is_object_like && c->count > 1) {

            /* form filter */

            memset(filter, 0, sizeof(Flags) * shape_subdivs); /* reset filter */

            static int outermost_shape_indices[MAX_ELEM_REFS];
            dvec centroid = mesh_polygonize_shape_bundle(c->shapes, c->count, shape_subdivs, verts);

            for (int subdiv_i = 0; subdiv_i < shape_subdivs; ++subdiv_i) {
                int count = mesh_find_outermost_shapes_for_subdivision(verts, centroid, subdiv_i, subdiv_da, c->count, outermost_shape_indices);

                for (int j = 0; j < count; ++j) {
                    Shape *shape = c->shapes[outermost_shape_indices[j]];
                    if (t_is_object_like)
                        flags_add(filter + subdiv_i, shape->ids.nose);
                    else
                        flags_add(filter + subdiv_i, shape->ids.tail);
                }
            }

            /* apply filter to mark mesh points as outermost or not */

            if (t_is_object_like) { /* tailwise merge */

                for (int i = 0; i < n_env->slices_count; ++i) {
                    MeshEnvSlice *slice = n_env->slices + i;

                    if (slice->ids.tail != object_like_i)
                        continue;

                    Flags conn_flag = flags_make(slice->ids.nose);

                    for (int j = slice->beg; ; j = period_incr(j, n_env->count)) {
                        MeshPoint *p = n_env->points + j;

                        if (!flags_and(filter + p->subdiv_i, &conn_flag))
                            p->t_is_outermost = false;

                        if (j == slice->end)
                            break;
                    }
                }
            }
            else {                  /* nosewise merge */

                for (int i = 0; i < t_env->slices_count; ++i) {
                    MeshEnvSlice *slice = t_env->slices + i;

                    if (slice->ids.nose != object_like_i)
                        continue;

                    Flags conn_flag = flags_make(slice->ids.tail);

                    for (int j = slice->beg; ; j = period_incr(j, t_env->count)) {
                        MeshPoint *p = t_env->points + j;

                        if (!flags_and(filter + p->subdiv_i, &conn_flag))
                            p->n_is_outermost = false;

                        if (j == slice->end)
                            break;
                    }
                }
            }
        }
    }

    arena->unlock();
    arena->unlock();
    arena->unlock();
}

/* Sample vertices for given shapes, putting all vertices for a subdivision next to each other. */
dvec mesh_polygonize_shape_bundle(Shape **shapes, int shapes_count, int shape_subdivs, dvec *verts) {
    int curve_subdivs = shape_subdivs / SHAPE_CURVES;
    double dt = 1.0 / curve_subdivs;

    for (int prev_curve_i = 0; prev_curve_i < SHAPE_CURVES; ++prev_curve_i) {
        int next_curve_i = (prev_curve_i + 1) % SHAPE_CURVES;

        for (int curve_subdiv_i = 0; curve_subdiv_i < curve_subdivs; ++curve_subdiv_i) {
            int subdiv_base = (prev_curve_i * curve_subdivs + curve_subdiv_i) * shapes_count;
            dvec *subdiv_verts = verts + subdiv_base;
            double t = curve_subdiv_i * dt;

            for (int shape_i = 0; shape_i < shapes_count; ++shape_i) {
                Shape *shape = shapes[shape_i];
                Curve *curve1 = shape->curves + prev_curve_i;
                Curve *curve2 = shape->curves + next_curve_i;

                subdiv_verts[shape_i] = shape_bezier(curve1->x, curve1->y,
                                                     curve1->cx, curve1->cy,
                                                     curve2->x, curve2->y,
                                                     t, curve1->w);
            }
        }
    }

    /* calculate bundle centroid */

    dvec centroid;
    centroid.x = 0.0;
    centroid.y = 0.0;

    for (int i = 0; i < shapes_count; ++i) {
        dvec c = shape_centroid(shapes[i]);
        centroid.x += c.x;
        centroid.y += c.y;
    }

    centroid.x /= (double)shapes_count;
    centroid.y /= (double)shapes_count;

    return centroid;
}

/* Identifies shapes (by index) whose point for the given subdivision is outermost. Returns number of found
shapes since multiple shapes can have outermost points. */
int mesh_find_outermost_shapes_for_subdivision(dvec *verts, dvec centroid, int subdiv_i, double subdiv_da, int shapes_count, int *outermost_shape_indices) {
    dvec *subdiv_verts = verts + subdiv_i * shapes_count;
    double subdiv_a = subdiv_i * subdiv_da;
    double nx = cos(subdiv_a);
    double ny = sin(subdiv_a);

    /* calculate points' dot (outermostness) and sort them */

    static struct _Dot {
        double dot;
        int index;
    } vert_dots[MAX_ELEM_REFS];

    for (int i = 0; i < shapes_count; ++i) {
        double vx = subdiv_verts[i].x - centroid.x;
        double vy = subdiv_verts[i].y - centroid.y;
        double dot = nx * vx + ny * vy;

        int ins_i = 0;
        for (; ins_i < i; ++ins_i) /* find insertion point */
            if (vert_dots[ins_i].dot < dot)
                break;

        for (int j = i; j > ins_i; --j) /* make room for insertion */
            vert_dots[j] = vert_dots[j - 1];

        /* insert new dot */
        vert_dots[ins_i].dot = dot;
        vert_dots[ins_i].index = i;
    }

    /* determine how many of the outermost points are actually suitable */

    outermost_shape_indices[0] = vert_dots[0].index; /* first outermost point is automatically added */
    int outermost_count = 1;

    for (int i = 1; i < shapes_count; ++i) /* find runners up that are close enough to the first point */
        if (fabs(vert_dots[0].dot - vert_dots[i].dot) < _OUTERMOST_MARGIN)
            outermost_shape_indices[outermost_count++] = vert_dots[i].index;
        else
            break;

    return outermost_count;
}
