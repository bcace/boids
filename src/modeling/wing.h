#ifndef wing_h
#define wing_h

#include "../mantle.h"
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
    float taper; /* tip chord / root chord */
    float span;
    float aoa;
    float sweep; /* leading edge angle, degrees */
    float dihedral; /* degrees */
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

#endif
