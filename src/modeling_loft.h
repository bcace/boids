#ifndef modeling_loft_h
#define modeling_loft_h

#include "modeling_constants.h"
#include "modeling_id.h"
#include "modeling_shape.h"
#include "math_dvec.h"

#define MAX_FUSELAGE_CONNS 32


struct Wing;
struct Arena;
struct Model;
struct Object;

struct StationId {
    short int id; /* required station id, -1 for filler stations */
    short int index; /* station index when all stations are created */
};

struct Oref {
    Object *object;
    bool is_clone;

    /* data derived from object */
    double x, y, z; /* position, model CS */
    Former t_skin_former, n_skin_former; /* skin formers, model CS */

    /* used for lofting */
    short int t_conns_count;
    short int n_conns_count;

    union {
        struct { /* used before lofting */
            Flags non_clone_origin; /* origin flag where original object and all its clones have the same origin */
            struct {
                Flags conn_flags; /* all connected objrefs, in unique origins */
                Flags non_clone_origins; /* all connected objrefs, in non-clone origins */
            } t, n;
        };
        struct { /* used for lofting */
            tvec t_tangents[SHAPE_CURVES];
            tvec n_tangents[SHAPE_CURVES];
            Id id; /* object lofting id */
        };
    };

    StationId t_station, n_station;
};

struct Wisec {
    int i; /* starting envelope point index of the segment where the intersection is */
    double t;
    tvec p;
};

struct Wref {
    Wing *wing;
    bool is_clone;
    Id id;

    /* derived */
    StationId t_station, n_station;
    bool isec_valid; /* is intersection with fuselage valid */
};

struct Conn {
    Oref *tail_o;
    Oref *nose_o;
};

struct Fuselage {
    Oref orefs[MAX_ELEM_REFS];
    int orefs_count;
    Wref wrefs[MAX_ELEM_REFS];
    int wrefs_count;
    Conn conns[MAX_FUSELAGE_CONNS];
    int conns_count;
};

/* loft */
void fuselage_update_conns(Arena *arena, Fuselage *fuselage);
void fuselage_update_longitudinal_tangents(Fuselage *fuselage);
void fuselage_loft(Arena *arena, Arena *verts_arena, Model *model, Fuselage *fuselage);
bool fuselage_objects_overlap(Oref *a, Oref *b);
bool fuselage_object_and_wing_overlap(Oref *o, Wref *w);

void loft_fuselage_wing_intersections(Arena *arena, Wref *wrefs, int wrefs_count, struct TraceSection *sections, int sections_count);

#define MAX_WING_ISECS_PER_STATION (MAX_ELEM_REFS * 2)


/* Trace envelope point. */
struct EnvPoint {
    double x, y;
    double t1, t2;  /* polygon edge t values, non-zero only for intersections */
    int i1, i2;     /* polygon vertex indices */
    Ids ids;        /* second shape origin for intersections */
    int subdiv_i;   /* starting polygon vertex index of polygon side where the point is located */
    bool is_intersection;
};

/* Trace envelope, generated initially when tracing around the fuselage. */
struct TraceEnv {
    EnvPoint points[MAX_ENVELOPE_POINTS];
    int count;
    Flags object_like_flags; /* object-like polygons forming this envelope, passed down to corresponding mesh envelope */
};

/* Section containing all pipe intersection shapes and resulting envelopes. Mostly contains
only one set of shapes and a single envelope, and two shape sets and two envelopes if there's
a possibility of an opening. */
struct TraceSection {
    Shape shapes[MAX_ENVELOPE_SHAPES];      /* actual storage */
    Shape *t_shapes[MAX_ENVELOPE_SHAPES];   /* aliases, shapes in tailwise direction */
    Shape *n_shapes[MAX_ENVELOPE_SHAPES];   /* aliases, shapes in nosewise direction */
    int shapes_count;
    int t_shapes_count;
    int n_shapes_count;
    bool two_envelopes;
    TraceEnv *t_env, *n_env; /* pointers because they migh point at the same thing in arena */
    double x;
    Wisec wisecs[MAX_WING_ISECS_PER_STATION];
    int wisecs_count;
};

/* Mesh point, representing actual skin vertex and containing all info required for meshing. */
struct MeshPoint {
    double x, y;
    int i1, i2;     /* polygon vertex indices */
    Ids ids;        /* second shape origin for intersections */
    int subdiv_i;   /* starting polygon vertex index of polygon side where the point is located */
    int vert_i;     /* index of vertex created from the point, offset from envelope verts_base_i */
    bool is_intersection;
    bool t_is_outermost, n_is_outermost; /* result of merge filter */
};

struct MeshEnvSlice {
    int beg, end; /* envelope point indices, [beg, end] */
    Ids ids;
};

/* Additional vertex info required for meshing, generated from a corresponding trace envelope. */
struct MeshEnv {
    MeshPoint points[MAX_ENVELOPE_POINTS];
    int count;
    MeshEnvSlice slices[MAX_ENVELOPE_POINTS];
    int slices_count;
    int verts_base_i;           /* index of first vertex in model vertex buffer so we can map envelope points to their vertices */
    float x;                    /* section x, model CS */
    Flags object_like_flags;    /* used for creating merge filter */
};

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
