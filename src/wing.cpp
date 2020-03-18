#include "wing.h"
#include "debug.h"
#include <math.h>
#include <stdlib.h>


void wing_init(Wing *w) {
    w->spars_count = 0;
    wing_add_spar(w, 0.25);
    wing_add_spar(w, 0.7);
}

void wing_add_spar(Wing *w, float x) {
    break_assert(w->spars_count < WING_MAX_SPARS);
    break_assert(x > 0.05 && x < 0.95);
    w->spars[w->spars_count++].x = x;
}

int wing_get_required_stations(Wing *w, float *stations) {
    int c = 0;
    stations[c++] = w->x - w->chord * 0.5; /* first station */
    for (int i = 0; i < w->spars_count; ++i)
        stations[c++] = w->x + w->chord * w->spars[i].x;
    stations[c++] = w->x + w->chord * 0.5; /* last station */
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
    wing_init(w);
    w->x = x;
    w->y = y;
    w->z = z;
    w->airfoil = airfoils_base[index];
    return w;
}
