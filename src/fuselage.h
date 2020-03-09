#ifndef fuselage_h
#define fuselage_h

#include "dvec.h"
#include "shape.h"
#include "config.h"
#include "element.h"
#include "constants.h"

#define MAX_FUSELAGE_CONNS  32


struct Arena;
struct Model;
struct Object;

struct Objref {
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
};

struct Conn {
    Objref *tail_o;
    Objref *nose_o;
};

struct Fuselage {
    Objref objects[MAX_ELEM_REFS];
    int objects_count;
    Conn conns[MAX_FUSELAGE_CONNS];
    int conns_count;
};

void fuselage_update_conns(Arena *arena, Fuselage *fuselage);
void fuselage_update_longitudinal_tangents(Fuselage *fuselage);
void fuselage_loft(Arena *arena, Arena *verts_arena, Model *model, Fuselage *fuselage);

#endif
