#ifndef fuselage_h
#define fuselage_h

#include "dvec.h"
#include "shape.h"
#include "config.h"
#include "element.h"
#include "constants.h"

#define MAX_FUSELAGE_CONNS  32


struct Wing;
struct Arena;
struct Model;
struct Object;
struct TraceEnv;

struct FuselageIsec {
    int i; /* starting envelope point index of the segment where the intersection is */
    double t;
    tvec p;
};

struct StationId {
    short int id;
    short int index;
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
    FuselageIsec t_isec, n_isec;
    tvec *r_cut, *t_cut; /* wing prism used to cut fuselage, root/tip points */

    StationId t_station, n_station;
};

struct Conn {
    Oref *tail_o;
    Oref *nose_o;
};

struct TraceShapes {
    Shape shapes[MAX_ENVELOPE_SHAPES]; /* actual storage */
    Shape *t_shapes[MAX_ENVELOPE_SHAPES]; /* aliases, shapes looking tailwise */
    Shape *n_shapes[MAX_ENVELOPE_SHAPES]; /* aliases, shapes looking nosewise */
    int shapes_count;
    int t_shapes_count;
    int n_shapes_count;
    bool two_envelopes;
};

enum _StationType { ST_NONE, ST_TAIL, ST_NOSE };

/* Marks position of fuselage sections. */
struct _Station {
    float x;
    short int id;
    _StationType type;
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
void fuselage_get_shapes_at_station(Fuselage *fuselage, _Station *station, TraceShapes *shapes);

/* wing */
// ...

#endif
