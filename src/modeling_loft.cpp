#include "modeling_loft.h"
#include "modeling_object.h"
#include "modeling_wing.h"
#include "math_math.h"
#include <math.h>


/* Circle used to roughly describe object cross-section for grouping objects into fuselages. */
struct Circle {
    double x, y, r;
};

/* Approximation of the area in y-z plane shadowed by the object. */
static Circle _object_circle(Object *o, bool is_clone) {
    Circle c;
    double rx = ((double)o->max_y - (double)o->min_y) * 0.5;
    double ry = ((double)o->max_z - (double)o->min_z) * 0.5;
    c.x = o->min_y + rx;
    c.y = o->min_z + ry;
    c.r = max_d(rx, ry) + 0.05; /* a small margin is added to the circle */
    if (is_clone)
        c.x = -c.x;
    return c;
}

static bool _circles_overlap(double x1, double y1, double r1,
                             double x2, double y2, double r2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double l = sqrt(dx * dx + dy * dy); // FAST_SQRT
    return l < r1 + r2;
}

/* Returns true if circles representing objects in the y-z plane overlap. */
bool fuselage_objects_overlap(Oref *a, Oref *b) {
    Circle ca = _object_circle(a->object, a->is_clone);
    Circle cb = _object_circle(b->object, b->is_clone);
    return _circles_overlap(ca.x, ca.y, ca.r,
                            cb.x, cb.y, cb.r);
}

/* Returns true if exactly one of the wing's formers overlaps with the
referenced object in the y-z plane. */
bool fuselage_object_and_wing_overlap(Oref *o, Wref *w) {
    Circle c = _object_circle(o->object, o->is_clone);
    Wing *wing = w->wing;
    return _circles_overlap(c.x, c.y, c.r,
                            wing->y + wing->def.r_former.y, wing->z + wing->def.r_former.z, 0.0) !=
           _circles_overlap(c.x, c.y, c.r,
                            wing->y + wing->def.t_former.y, wing->z + wing->def.t_former.z, 0.0);
}
