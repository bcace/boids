#ifndef fuselage_h
#define fuselage_h

#include "modeling_shape.h"
#include "modeling_config.h"
#include "modeling_element.h"
#include "modeling_constants.h"
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

struct Wref {
    Wing *wing;
    bool is_clone;
    Id id;

    /* derived */
    StationId t_station, n_station;
    bool valid;
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

/* wing */
// void fuselage_wing_intersections(Arena *arena, Wref *wrefs, int wrefs_count, struct TraceSection *sections, int sections_count);

#endif
