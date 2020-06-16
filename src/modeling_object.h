#ifndef object_h
#define object_h

#include "math_vec.h"
#include "modeling_shape.h"
#include "modeling_constants.h"

#define MAX_OBJECT_FORMERS      8
#define MAX_SKIN_FORMERS        4

#define COLL_VERTS_PER_CURVE    4
#define COLL_VERTS_PER_PRISM    (SHAPE_CURVES * COLL_VERTS_PER_CURVE)


struct ObjectDef {
    Former formers[MAX_OBJECT_FORMERS];     /* collision formers */
    int formers_count;                      /* collision formers */
    Former t_skin_former, n_skin_former;    /* skin formers, object CS */
    float t_endp_dx, n_endp_dx;             /* endpoint distances from respective skin formers */
};

struct Object {
    /* serialize */
    vec3 p;
    ObjectDef def;

    /* collision */
    vec3 f;
    struct CollPrism *prisms; /* allocated in arena */
    int prisms_count;
    float min_x, max_x; /* model CS */
    float min_y, max_y; /* model CS, collision formers */
    float min_z, max_z; /* model CS, collision formers */
    float coll_min_x, coll_max_x; /* with refs */

    /* control */
    bool selected;
    vec3 drag_p;
};

void object_move(Object *o, vec3 dp);
void object_reset_drag_p(Object *o);
void object_finish(Object *o);
void object_update_extents(Object *o);
bool object_should_be_centered(Object *o);
bool object_should_be_mirrored(Object *o);

#endif
