#include "wing.h"
#include "debug.h"
#include <math.h>


void wing_init(Wing *w) {
    w->spars_count = 0;
}

void wing_add_spar(Wing *w, float x) {
    break_assert(x > 0.05 && x < 0.95);
    Spar *s = w->spars + w->spars_count++;
    s->x = x;
}

void wing_update(Wing *w, float dx_hint) {
    double tx = w->x - w->i_length * 0.5;
    double lx = w->x + w->i_length * 0.5;

    // TODO: create logitudinal wing stations, between and including spar locations

    /* TODO: for each station find ideal airfoil samples, translate and rotate them.
    These will be points intersecting lines will go through. */
}
