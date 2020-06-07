#include "modeling_wing.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>


void wing_add_spar(Wing *w, float x) {
    assert(w->spars_count < WING_MAX_SPARS);
    assert(x > 0.05 && x < 0.95);
    assert(w->spars_count == 0 || x > w->spars[w->spars_count - 1].x);
    w->spars[w->spars_count++].x = x;
}

/* Returns required stations' x positions, from trailing to leading edge.
Includes stations for edges and spars. */
int wing_get_required_stations(Wing *w, float *stations) {
    int c = 0;
    stations[c++] = w->x - w->chord; /* trailing station */
    for (int i = w->spars_count - 1; i <= 0; --i)
        stations[c++] = w->x - w->chord * w->spars[i].x;
    stations[c++] = w->x; /* leading station */
    return c;
}

bool wing_should_be_centered(Wing *w) {
    return w->y < WING_SNAP_WIDTH;
}

bool wing_should_be_mirrored(Wing *w) {
    return !wing_should_be_centered(w);
}

Wing *wing_make_from_selected_base_airfoil(int index, float x, float y, float z) {
    Wing *w = (Wing *)malloc(sizeof(Wing));

    w->x = x;
    w->y = y;
    w->z = z;
    w->chord = 1.0f;
    w->taper = 0.5f;
    w->span = 10.0f;
    w->aoa = 0.0f;
    w->sweep = 20.0f;
    w->dihedral = 0.0f;
    w->spars_count = 0;

    w->selected = false;
    wing_add_spar(w, 0.25f);
    wing_add_spar(w, 0.7f);
    w->airfoil = airfoils_base[index];
    wing_reset_target_position(w);

    return w;
}

void wing_move_target_position(Wing *w, float dx, float dy, float dz) {
    w->tx += dx;
    w->ty += dy;
    w->tz += dz;
}

void wing_reset_target_position(Wing *w) {
    w->tx = w->x;
    w->ty = w->y;
    w->tz = w->z;
}