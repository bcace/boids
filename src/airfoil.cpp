#include "airfoil.h"
#include "dvec.h"
#include <string.h>


const double _AIRFOIL_DT = 1.0 / AIRFOIL_X_SUBDIVS;

double airfoil_get_subdiv_x(int i) {
    double t = (i + 1) * _AIRFOIL_DT;
    return t * t;
}

dvec airfoil_get_point(Airfoil *airfoil, int i) {
    dvec p;

    if (i < AIRFOIL_X_SUBDIVS) {
        AirfoilSide *side = &airfoil->upper;
        int j = AIRFOIL_X_SUBDIVS - i - 1;
        p.x = airfoil_get_subdiv_x(j);
        p.y = side->base + side->y[j] * side->delta;
    }
    else if (i == AIRFOIL_X_SUBDIVS) {
        p.x = 0.0;
        p.y = 0.0;
    }
    else {
        AirfoilSide *side = &airfoil->lower;
        int j = i - AIRFOIL_X_SUBDIVS - 1;
        p.x = airfoil_get_subdiv_x(j);
        p.y = side->base + side->y[j] * side->delta;
    }

    return p;
}

void airfoil_init(Airfoil *airfoil,
                  float u_base, float u_delta, unsigned char *u_y,
                  float l_base, float l_delta, unsigned char *l_y) {

    memcpy(airfoil->upper.y, u_y, sizeof(unsigned char) * AIRFOIL_X_SUBDIVS);
    airfoil->upper.base = u_base;
    airfoil->upper.delta = u_delta;

    memcpy(airfoil->lower.y, l_y, sizeof(unsigned char) * AIRFOIL_X_SUBDIVS);
    airfoil->lower.base = l_base;
    airfoil->lower.delta = l_delta;
}

float _get_side_trailing_offset(AirfoilSide *s) {
    return s->base + s->y[AIRFOIL_X_SUBDIVS - 1] * s->delta;
}

float airfoil_get_trailing_y_offset(Airfoil *a) {
    return (_get_side_trailing_offset(&a->upper) +
            _get_side_trailing_offset(&a->lower)) * 0.5f;
}
