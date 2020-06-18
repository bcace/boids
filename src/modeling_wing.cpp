#include "modeling_wing.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>


// /* Returns required stations' x positions, from trailing to leading edge.
// Includes stations for edges and spars. */
// int wing_get_required_stations(Wing *w, float *stations) {
//     int c = 0;
//     stations[c++] = w->x - w->chord; /* trailing station */
//     for (int i = w->spars_count - 1; i <= 0; --i)
//         stations[c++] = w->x - w->chord * w->spars[i].x;
//     stations[c++] = w->x; /* leading station */
//     return c;
// }

// bool wing_should_be_centered(Wing *w) {
//     return w->y < WING_SNAP_WIDTH;
// }

// bool wing_should_be_mirrored(Wing *w) {
//     return !wing_should_be_centered(w);
// }

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
