#ifndef mesh_h
#define mesh_h

#include "constants.h"


struct EnvPoint {
    double x, y;
    double t1, t2;  /* polygon edge t values, non-zero only for intersections */
    int i1, i2;     /* polygon vertex indices */
    Origin origin;  /* second shape origin for intersections */
    int subdiv_i;   /* starting polygon vertex index of polygon side where the point is located */
    bool is_intersection;
};

// TODO: think about merging this with EnvPoint struct
struct MeshPoint {
    double x, y;
    int i1, i2;     /* polygon vertex indices */
    Origin origin;  /* second shape origin for intersections */
    int subdiv_i;   /* starting polygon vertex index of polygon side where the point is located */
    int vert_i;     /* index of vertex created from the point, offset from envelope verts_base_i */
    bool is_intersection;
};

struct MeshEnvSlice {
    int beg, end; /* envelope point indices, [beg, end] */
    Origin origin;
};

struct MeshEnv {
    MeshPoint points[SHAPE_MAX_ENVELOPE_POINTS];
    int count;
    MeshEnvSlice slices[SHAPE_MAX_ENVELOPE_POINTS];
    int slices_count;
    int verts_base_i; /* index of first vertex in model vertex buffer so we can map envelope points to their vertices */
    float x; /* section x, model CS */
    OriginFlags object_like_flags; /* used for creating merge filter */
};

struct MergeFilter {
    OriginFlags conns[SHAPE_MAX_SHAPE_SUBDIVS];
    bool active;
};

struct MergeFilters {
    MergeFilter objects[MAX_FUSELAGE_OBJECTS];
};

struct dvec;
struct Model;
struct Shape;
struct Arena;

/* filter */
MergeFilters *mesh_init_filter(Arena *arena);
void mesh_polygonize_shape_bundle(Shape **shapes, int shapes_count, int shape_subdivs, dvec *verts);
int mesh_find_outermost_shapes_for_subdivision(dvec *verts, int subdiv_i, double subdiv_da, int shapes_count, int *outermost_shape_indices);
void mesh_make_merge_filter(MergeFilters *filters, int shape_subdivs,
                            Shape **t_shapes, int t_shapes_count, MeshEnv *t_env,
                            Shape **n_shapes, int n_shapes_count, MeshEnv *n_env);

/* trace */
int mesh_trace_envelope(EnvPoint *env_points, Shape **shapes, int shapes_count, int curve_subdivs, OriginFlags *object_like_flags);

/* envelope */
void mesh_make_envelopes(Model *model, Arena *verts_arena, float section_x,
                         MeshEnv *t_env, Shape **t_shapes, int t_shapes_count, struct EnvPoint *t_env_points,
                         MeshEnv *n_env, Shape **n_shapes, int n_shapes_count, struct EnvPoint *n_env_points);

/* mesh */
void mesh_init(Model *model);
void mesh_between_two_sections(Model *model, int shape_subdivs, MergeFilters *filters,
                               MeshEnv *t_env, int *t_neighbors_map,
                               MeshEnv *n_env, int *n_neighbors_map);
void mesh_verts_merge_margin(bool increase); /* just for debugging */

#endif
