#ifndef mesh_h
#define mesh_h

#include "modeling_constants.h"
#include "modeling_element.h"


struct EnvPoint {
    double x, y;
    double t1, t2;  /* polygon edge t values, non-zero only for intersections */
    int i1, i2;     /* polygon vertex indices */
    Ids ids;        /* second shape origin for intersections */
    int subdiv_i;   /* starting polygon vertex index of polygon side where the point is located */
    bool is_intersection;
};

struct TraceEnv {
    EnvPoint points[MAX_ENVELOPE_POINTS];
    int count;
    Flags object_like_flags;
};

// TODO: think about merging this with EnvPoint struct
struct MeshPoint {
    double x, y;
    int i1, i2;     /* polygon vertex indices */
    Ids ids;        /* second shape origin for intersections */
    int subdiv_i;   /* starting polygon vertex index of polygon side where the point is located */
    int vert_i;     /* index of vertex created from the point, offset from envelope verts_base_i */
    bool is_intersection;
    bool t_is_outermost;
    bool n_is_outermost;
};

struct MeshEnvSlice {
    int beg, end; /* envelope point indices, [beg, end] */
    Ids ids;
};

struct MeshEnv {
    MeshPoint points[MAX_ENVELOPE_POINTS];
    int count;
    MeshEnvSlice slices[MAX_ENVELOPE_POINTS];
    int slices_count;
    int verts_base_i; /* index of first vertex in model vertex buffer so we can map envelope points to their vertices */
    float x; /* section x, model CS */
    Flags object_like_flags; /* used for creating merge filter */
};

struct dvec;
struct Model;
struct Shape;
struct Arena;

/* filter */
dvec mesh_polygonize_shape_bundle(Shape **shapes, int shapes_count, int shape_subdivs, dvec *verts);
int mesh_find_outermost_shapes_for_subdivision(dvec *verts, dvec centroid, int subdiv_i, double subdiv_da, int shapes_count, int *outermost_shape_indices);
void mesh_apply_merge_filter(Arena *arena, int shape_subdivs,
                             Shape **t_shapes, int t_shapes_count, MeshEnv *t_env,
                             Shape **n_shapes, int n_shapes_count, MeshEnv *n_env);

/* trace */
bool mesh_trace_envelope(TraceEnv *env, Shape **shapes, int shapes_count, int curve_subdivs);

/* envelope */
void mesh_make_envelopes(Model *model, Arena *verts_arena, float section_x,
                         MeshEnv *t_env, TraceEnv *t_trace_env,
                         MeshEnv *n_env, TraceEnv *n_trace_env);

/* mesh */
void mesh_init(Model *model);
void mesh_between_two_sections(Model *model, int shape_subdivs,
                               MeshEnv *t_env, int *t_neighbors_map,
                               MeshEnv *n_env, int *n_neighbors_map);
void mesh_verts_merge_margin(bool increase); /* just for debugging */

#endif
