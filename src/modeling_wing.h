#ifndef modeling_wing_h
#define modeling_wing_h

#include "modeling_id.h"
#include "modeling_airfoil.h"

#define WING_SNAP_WIDTH 0.05


struct WFormer {
    /* serialize */
    Airfoil airfoil;
    float aoa;      /* rad */
    float chord;
    float x, y, z;  /* wing CS */
};

struct WingDef {
    WFormer r_former, t_former;
};

struct Wing {
    /* serialize */
    float x, y, z;  /* model CS */
    WingDef def;

    /* control */
    float tx, ty, tz; /* target position, used for dragging */
    float fx, fy, fz; /* forces acting on wing during collision */
    bool selected;
};

float wing_get_nominal_root_chord(Wing *w);
int wing_get_required_stations(Wing *w, float *stations);
// bool wing_should_be_centered(Wing *w);
// bool wing_should_be_mirrored(Wing *w);
void wing_move_target_position(Wing *w, float dx, float dy, float dz);
void wing_reset_target_position(Wing *w);

#endif
