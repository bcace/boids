#ifndef fuselage_h
#define fuselage_h

#include "vec.h"
#include "shape.h"
#include "config.h"
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

    /* only used while grouping objects into fuselages */
    int fuselage_id;

    /* used for lofting */
    int t_conns_count;
    int n_conns_count;
    vec3 t_tangents[SHAPE_CURVES];
    vec3 n_tangents[SHAPE_CURVES];
    OriginPart origin; /* object lofting id */
};

struct Conn {
    Objref *tail_o;
    Objref *nose_o;
};

struct Fuselage {
    Objref objects[MAX_FUSELAGE_OBJECTS];
    int objects_count;
    Conn conns[MAX_FUSELAGE_CONNS];
    int conns_count;
};

void fuselage_loft(Arena *arena, Arena *verts_arena, Model *model, Fuselage *fuselage);

#endif
