#ifndef modeling_wing_h
#define modeling_wing_h

#include "modeling_id.h"
#include "modeling_airfoil.h"

#define WING_MAX_SPARS  32
#define WING_SNAP_WIDTH 0.05


struct Spar {
    float x; /* fraction of LE - TE, [0, 1] */
};

struct WFormer {
    /* serialize */
    Airfoil airfoil;
    float aoa;      /* rad */
    float chord;
    float x, y, z;  /* wing CS */
};

struct WingDef {
    WFormer r_former, t_former;
    Spar spars[WING_MAX_SPARS]; /* defined from leading to trailing edge */
    int spars_count;
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

// int wing_get_required_stations(Wing *w, float *stations);
// bool wing_should_be_centered(Wing *w);
// bool wing_should_be_mirrored(Wing *w);
// Wing *wing_make_from_selected_base_airfoil(int index, float x, float y, float z);
// void wing_move_target_position(Wing *w, float dx, float dy, float dz);
// void wing_reset_target_position(Wing *w);

#endif
