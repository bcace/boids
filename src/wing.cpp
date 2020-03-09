#include "wing.h"
#include "debug.h"
#include <math.h>


void wing_init(Wing *w) {
    w->spars_count = 0;
}

void wing_add_spar(Wing *w, float x) {
    break_assert(w->spars_count < WING_MAX_SPARS);
    break_assert(x > 0.05 && x < 0.95);
    w->spars[w->spars_count++].x = x;
}

int wing_get_required_stations(Wing *w, float *stations) {
    int c = 0;
    stations[c++] = w->x - w->i_length * 0.5; /* first station */
    for (int i = 0; i < w->spars_count; ++i)
        stations[c++] = w->x + w->i_length * w->spars[i].x;
    stations[c++] = w->x + w->i_length * 0.5; /* last station */
    return c;
}
