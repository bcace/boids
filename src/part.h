#ifndef part_f
#define part_f

#include "vec.h"
#include "shape.h"

#define MAX_PROTO_NAME          128
#define MAX_PROTO_FORMERS       8


struct Object;

struct Part {
    char name[MAX_PROTO_NAME];

    /* data copied onto objects */
    // TODO: think about making this a separate structure, shared between Object and Part
    // TODO: don't use array here
    Former formers[MAX_PROTO_FORMERS]; /* initial collision formers */
    int formers_count;
    Former tail_skin_former, nose_skin_former;
    float tail_endp_dx, nose_endp_dx; /* endpoint distances from respective skin formers */

    bool tail_opening, nose_opening;
};

void part_init(Part *part, const char *name, float tail_endp_dx, float nose_endp_dx, bool tail_opening, bool nose_opening);
void part_add_coll_former(Part *part, Shape shape, float x);
void part_set_skin_formers(Part *part, Shape tail_shape, float tail_x, Shape nose_shape, float nose_x);
Object *part_make_object(Part *part, vec3 p);

#endif
