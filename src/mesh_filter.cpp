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
    Shape *t_shapes[MAX_FUSELAGE_OBJECTS];
    Shape *n_shapes[MAX_FUSELAGE_OBJECTS];
    int t_count;
    int n_count;
};

static _ConnsForObject *conns;
static dvec *verts;
static bool initialized = false;

/* Allocates storage for merge filters and memory for internal use. */
MergeFilters *mesh_init_filter(Arena *arena) {
    initialized = true;
    conns = arena->alloc<_ConnsForObject>(MAX_FUSELAGE_OBJECTS);
    verts = arena->alloc<dvec>(SHAPE_MAX_SHAPE_SUBDIVS * MAX_FUSELAGE_OBJECTS);
    return arena->alloc<MergeFilters>(1);
}

// TODO: document
void mesh_make_merge_filter(MergeFilters *filters, int shape_subdivs,
                            Shape **t_shapes, int t_shapes_count, OriginFlags t_object_like_flags,
                            Shape **n_shapes, int n_shapes_count, OriginFlags n_object_like_flags) {
    assert(initialized);
    memset(conns, 0, sizeof(_ConnsForObject) * MAX_FUSELAGE_OBJECTS); /* reset correlations between shapes */

    /* collect all connection shapes */

    for (int i = 0; i < t_shapes_count; ++i) {
        Shape *s = t_shapes[i];
        if (s->origin.tail != s->origin.nose) {
            _ConnsForObject *c = conns + s->origin.nose;
            c->t_shapes[c->t_count++] = s;
        }
    }

    for (int i = 0; i < n_shapes_count; ++i) {
        Shape *s = n_shapes[i];
        if (s->origin.tail != s->origin.nose) {
            _ConnsForObject *c = conns + s->origin.tail;
            c->n_shapes[c->n_count++] = s;
        }
    }

    /* find merge transitions among correlations */

    double subdiv_da = TAU / shape_subdivs;

    for (int i = 0; i < MAX_FUSELAGE_OBJECTS; ++i) {
        _ConnsForObject *c = conns + i;
        MergeFilter *filter = filters->objects + i;

        OriginFlags f = ORIGIN_PART_TO_FLAG(i);
        bool t_is_object_like = t_object_like_flags & f;
        bool n_is_object_like = n_object_like_flags & f;

        for (int subdiv_i = 0; subdiv_i < shape_subdivs; ++subdiv_i) /* reset conn flags */
            filter->conns[subdiv_i] = 0ull;

        static int outermost_shape_indices[MAX_FUSELAGE_OBJECTS];

        if (t_is_object_like && c->n_count > 1) { /* tailwise merge transition */
            mesh_polygonize_shape_bundle(c->n_shapes, c->n_count, shape_subdivs, verts);

            for (int subdiv_i = 0; subdiv_i < shape_subdivs; ++subdiv_i) {
                int count = mesh_find_outermost_shapes_for_subdivision(verts, subdiv_i, subdiv_da, c->n_count, outermost_shape_indices);
                for (int j = 0; j < count; ++j) {
                    int shape_i = outermost_shape_indices[j];
                    filter->conns[subdiv_i] |= ORIGIN_PART_TO_FLAG(c->n_shapes[shape_i]->origin.nose);
                }
            }

            filter->active = true;
        }
        else if (n_is_object_like && c->t_count > 1) { /* nosewise merge transition */
            mesh_polygonize_shape_bundle(c->t_shapes, c->t_count, shape_subdivs, verts);

            for (int subdiv_i = 0; subdiv_i < shape_subdivs; ++subdiv_i) {
                int count = mesh_find_outermost_shapes_for_subdivision(verts, subdiv_i, subdiv_da, c->t_count, outermost_shape_indices);
                for (int j = 0; j < count; ++j) {
                    int shape_i = outermost_shape_indices[j];
                    filter->conns[subdiv_i] |= ORIGIN_PART_TO_FLAG(c->t_shapes[shape_i]->origin.tail);
                }
            }

            filter->active = true;
        }
        else
            filter->active = false;
    }
}

/* Sample vertices for given shapes, putting all vertices for a subdivision next to each other. */
void mesh_polygonize_shape_bundle(Shape **shapes, int shapes_count, int shape_subdivs, dvec *verts) {
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
}

/* Identifies shapes (by index) whose point for the given subdivision is outermost. Returns number of found
shapes since multiple shapes can have outermost points. */
int mesh_find_outermost_shapes_for_subdivision(dvec *verts, int subdiv_i, double subdiv_da, int shapes_count, int *outermost_shape_indices) {
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
        dvec v = subdiv_verts[i];
        double dot = nx * v.x + ny * v.y;

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
