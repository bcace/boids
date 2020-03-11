#include "fuselage.h"
#include "object.h"
#include "wing.h"
#include "math.h"
#include <math.h>


/* Circle used to roughly describe object cross-section for grouping objects into fuselages. */
struct _Circle {
    double x, y, r;
};

/* Approximation of the area in y-z plane shadowed by the object. */
static _Circle _object_circle(Object *o, bool is_clone) {
    _Circle c;
    double rx = ((double)o->max_y - (double)o->min_y) * 0.5;
    double ry = ((double)o->max_z - (double)o->min_z) * 0.5;
    c.x = o->min_y + rx;
    c.y = o->min_z + ry;
    c.r = max_d(rx, ry) + 0.05; /* a small margin is added to the circle */
    if (is_clone)
        c.x = -c.x;
    return c;
}

/* Returns true if circles representing objects in the y-z plane overlap. */
bool fuselage_objects_overlap(Oref *a, Oref *b) {
    _Circle ca = _object_circle(a->object, a->is_clone);
    _Circle cb = _object_circle(b->object, b->is_clone);
    double dx = ca.x - cb.x;
    double dy = ca.y - cb.y;
    double l = sqrt(dx * dx + dy * dy); // FAST_SQRT
    return l < ca.r + cb.r;
}

bool fuselage_object_and_wing_overlap(Oref *o, Wref *w) {
    _Circle c = _object_circle(o->object, o->is_clone);
    double dx = w->wing->y - c.x;
    double dy = w->wing->z - c.y;
    double l = sqrt(dx * dx + dy * dy); // FAST_SQRT
    return l < c.r;
}
