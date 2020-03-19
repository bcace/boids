#ifndef collision_h
#define collision_h

#include "dvec.h"
#include "constants.h"

#define COLL_PRISM_VERTS (4 * SHAPE_CURVES)


struct Shape;
struct Arena;

struct CollPrism {
    dvec verts[COLL_PRISM_VERTS];
    double x, min_x, max_x;
};

struct CollContext {
    Arena *arena;
    bool dragging;
};

void coll_get_prism(Shape *shape, CollPrism *prism, double dx, double dy, double margin);
bool coll_interact_prisms(CollPrism *prism1, CollPrism *prism2, double *f);

#endif
