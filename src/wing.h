#ifndef wing_h
#define wing_h

#include "mantle.h"
#include "element.h"
#include "airfoil.h"

#define WING_MAX_SPARS  32
#define WING_SNAP_WIDTH 0.05


struct Spar {
    float x; /* fraction of LE - TE, [0, 1] */
};

struct Wing {
    /* serialize */
    float x, y, z; /* wing anchor position, places wing inside a fuselage */
    float chord; /* ideal root chord length */
    float dihedral;
    Spar spars[WING_MAX_SPARS]; /* defined from leading to trailing edge */
    int spars_count;
    Airfoil airfoil;

    /* control */
    float tx, ty, tz; /* target position, used for dragging */
    float fx, fy, fz; /* forces acting on wing during collision */
    bool selected;

    /* drawing */
    Mantle mantle;
};

void wing_add_spar(Wing *w, float x);
int wing_get_required_stations(Wing *w, float *stations);
bool wing_should_be_centered(Wing *w);
bool wing_should_be_mirrored(Wing *w);
Wing *wing_make_from_selected_base_airfoil(int index, float x, float y, float z);
void wing_move_target_position(Wing *w, float dx, float dy, float dz);
void wing_reset_target_position(Wing *w);

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

#endif
