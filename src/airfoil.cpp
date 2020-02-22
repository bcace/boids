#include "airfoil.h"
#include "dvec.h"


const double _AIRFOIL_DT = 1.0 / AIRFOIL_SUBDIVS;

double airfoil_get_subdiv_x(int i) {
    double t = (i + 1) * _AIRFOIL_DT;
    return t * t;
}

dvec airfoil_get_point(Airfoil *airfoil, int i) {
    dvec p;

    if (i < AIRFOIL_SUBDIVS) {
        AirfoilSide *side = &airfoil->upper;
        int j = AIRFOIL_SUBDIVS - i - 1;
        p.x = airfoil_get_subdiv_x(j);
        p.y = side->base + side->y[j] * side->delta;
    }
    else if (i == AIRFOIL_SUBDIVS) {
        p.x = 0.0;
        p.y = 0.0;
    }
    else {
        AirfoilSide *side = &airfoil->lower;
        int j = i - AIRFOIL_SUBDIVS - 1;
        p.x = airfoil_get_subdiv_x(j);
        p.y = side->base + side->y[j] * side->delta;
    }

    return p;
}
