#ifndef wing_h
#define wing_h

#include "dvec.h"
#include "airfoil.h"

#define WING_MAX_SPARS 32

/*

LE - leading edge.
TE - trailing edge.
LS - leading fuselage station.
TS - trailing fuselage station.
L - line segment between two anchors.

LS position is anchor plus half of chord.
To determine LE intersection with LS:
- Determine the plane using both anchors and a point that's unit distance from anchor and rotated by AOA around vector that's a projection of L in the x = anchor.x plane.
- Determine intersection line between that plane and x = LS.x plane.
- Intersect this line with LS envelope polygon.

LOFTING:
    1. Wing tells fuselage how many stations it requires.
    2. Fuselage fills in the rest of the stations and then requests a cutter from the wing at each station.
*/

/* Struct used for wing cutting through fuselage. */
struct WStation {
    union {
        struct {
            double lx, ly; /* left cutter point (origin of left line) */
            double rx, ry; /* right cutter point (origin of right line) */
        };
        struct { /* only in case of trailing or leading edge */
            double x, y;
        };
    };
    bool is_edge; /* is wing edge, contains only one point */
};

struct Spar {
    float x; /* fraction of LE - TE, [0, 1] */
};

struct Wing {
    float x, y, z; /* wing anchor position, places wing inside a fuselage */
    float i_length; /* ideal root chord length */
    float dihedral;
    Airfoil i_airfoil;
    Spar spars[WING_MAX_SPARS];
    int spars_count;

    /* derived */
};

void wing_init(Wing *w);
void wing_add_spar(Wing *w, float x);
int wing_get_required_stations(Wing *w, float *stations);

#endif
