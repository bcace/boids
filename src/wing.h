#ifndef wing_h
#define wing_h

#include "airfoil.h"

#define MAX_SPARS 32

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

*/

struct Spar {
    float x; /* fraction of LE - TE, [0, 1] */
};

struct Wing {
    float x, y, z; /* wing anchor position, places wing inside a fuselage */
    float i_length; /* ideal root chord length */
    float dihedral;
    Airfoil i_airfoil;

    Spar spars[MAX_SPARS];
    int spars_count;
};

void wing_init(Wing *w);
void wing_add_spar(Wing *w, float x);
void wing_update(Wing *w, float dx_hint);

#endif
