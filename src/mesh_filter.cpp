#include "mesh.h"
#include "shape.h"
#include "arena.h"
#include "dvec.h"
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
    Shape *shapes[MAX_FUSELAGE_OBJECTS];
    int count;
};

static _ConnsForObject *conns;
static dvec *verts;
static dvec *centroids;
static bool initialized = false;

/* Allocates storage for merge filters and memory for internal use. */
MergeFilters *mesh_init_filter(Arena *arena) {
    initialized = true;
    conns = arena->alloc<_ConnsForObject>(MAX_FUSELAGE_OBJECTS);
    verts = arena->alloc<dvec>(SHAPE_MAX_SHAPE_SUBDIVS * MAX_FUSELAGE_OBJECTS);
    centroids = arena->alloc<dvec>(MAX_FUSELAGE_OBJECTS);
    return arena->alloc<MergeFilters>(1);
}

// TODO: document
void mesh_make_merge_filter(MergeFilters *filters, int shape_subdivs,
                            Shape **t_shapes, int t_shapes_count, MeshEnv *t_env,
                            Shape **n_shapes, int n_shapes_count, MeshEnv *n_env) {
    assert(initialized);
    memset(conns, 0, sizeof(_ConnsForObject) * MAX_FUSELAGE_OBJECTS); /* reset correlations between shapes */

    /* collect all connection shapes */

    for (int i = 0; i < t_shapes_count; ++i) {
        Shape *s = t_shapes[i];
        if (s->origin.tail != s->origin.nose && (n_env->object_like_flags & ORIGIN_PART_TO_FLAG(s->origin.nose))) {
            _ConnsForObject *c = conns + s->origin.nose;
            c->shapes[c->count++] = s;
        }
    }

    for (int i = 0; i < n_shapes_count; ++i) {
        Shape *s = n_shapes[i];
        if (s->origin.tail != s->origin.nose && (t_env->object_like_flags & ORIGIN_PART_TO_FLAG(s->origin.tail))) {
            _ConnsForObject *c = conns + s->origin.tail;
            c->shapes[c->count++] = s;
        }
    }

    /* find merge transitions */

    double subdiv_da = TAU / shape_subdivs;

    for (int i = 0; i < MAX_FUSELAGE_OBJECTS; ++i) {
        _ConnsForObject *c = conns + i;

        OriginFlags flags = ORIGIN_PART_TO_FLAG(i);
        bool t_is_object_like = t_env->object_like_flags & flags;
        bool n_is_object_like = n_env->object_like_flags & flags;

        MergeFilter *filter = filters->objects + i;

        if (t_is_object_like == n_is_object_like)   /* not a transition, both sides are either object or conn */
            filter->active = false;
        else if (c->count <= 1)                     /* object only on one side, but conns are not multiple */
            filter->active = false;
        else {
            filter->active = true;

            for (int subdiv_i = 0; subdiv_i < shape_subdivs; ++subdiv_i) /* reset conn flags */
                filter->conns[subdiv_i] = 0ull;

            static int outermost_shape_indices[MAX_FUSELAGE_OBJECTS];
            mesh_polygonize_shape_bundle(c->shapes, c->count, shape_subdivs, verts, centroids);

            for (int subdiv_i = 0; subdiv_i < shape_subdivs; ++subdiv_i) {
                int count = mesh_find_outermost_shapes_for_subdivision(verts, centroids, subdiv_i, subdiv_da, c->count, outermost_shape_indices);

                for (int j = 0; j < count; ++j) {
                    Shape *shape = c->shapes[outermost_shape_indices[j]];
                    if (t_is_object_like)
                        filter->conns[subdiv_i] |= ORIGIN_PART_TO_FLAG(shape->origin.nose);
                    else
                        filter->conns[subdiv_i] |= ORIGIN_PART_TO_FLAG(shape->origin.tail);
                }
            }

            /* If we have slices we can set mesh points' is_outermost here.
            mesh_make_envelopes() should set is_outermost to true by default for all points. Then this procedure can set some of them to false here. */
        }
    }
}

/* Sample vertices for given shapes, putting all vertices for a subdivision next to each other. */
void mesh_polygonize_shape_bundle(Shape **shapes, int shapes_count, int shape_subdivs, dvec *verts, dvec *centroids) {
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

    for (int i = 0; i < shapes_count; ++i)
        centroids[i] = shape_centroid(shapes[i]);
}

/* Identifies shapes (by index) whose point for the given subdivision is outermost. Returns number of found
shapes since multiple shapes can have outermost points. */
int mesh_find_outermost_shapes_for_subdivision(dvec *verts, dvec *centroids, int subdiv_i, double subdiv_da, int shapes_count, int *outermost_shape_indices) {
    dvec *subdiv_verts = verts + subdiv_i * shapes_count;
    double subdiv_a = subdiv_i * subdiv_da;
    double nx = cos(subdiv_a);
    double ny = sin(subdiv_a);

    /* calculate points' dot (outermostness) and sort them */

    static struct _Dot {
        double dot;
        int index;
    } vert_dots[MAX_FUSELAGE_OBJECTS];

    for (int i = 0; i < shapes_count; ++i) {
        double vx = subdiv_verts[i].x - centroids[i].x;
        double vy = subdiv_verts[i].y - centroids[i].y;
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
