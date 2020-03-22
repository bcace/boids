#include "wing.h"
#include "debug.h"
#include <math.h>
#include <stdlib.h>


void wing_add_spar(Wing *w, float x) {
    break_assert(w->spars_count < WING_MAX_SPARS);
    break_assert(x > 0.05 && x < 0.95);
    break_assert(w->spars_count == 0 || x > w->spars[w->spars_count - 1].x);
    w->spars[w->spars_count++].x = x;
}

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
    w->selected = false;
    w->chord = 1.0f;
    w->spars_count = 0;
    wing_add_spar(w, 0.25f);
    wing_add_spar(w, 0.7f);
    w->x = x;
    w->y = y;
    w->z = z;
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
