#include "dvec.h"
#include <math.h>


dvec2 dvec_init(double x, double y) {
    dvec2 c;
    c.x = x;
    c.y = y;
    return c;
}

dvec2 dvec_plus(dvec2 a, dvec2 b) {
    dvec2 c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    return c;
}

dvec2 dvec_minus(dvec2 a, dvec2 b) {
    dvec2 c;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    return c;
}

dvec2 dvec_divide(dvec2 a, double b) {
    dvec2 c;
    c.x = a.x / b;
    c.y = a.y / b;
    return c;
}

double dvec_dot(dvec2 a, dvec2 b) {
    return a.x * b.x + a.y * b.y;
}

double dvec_cross(dvec2 a, dvec2 b) {
    return a.x * b.y - a.y * b.x;
}

double dvec_length(dvec2 a) {
    return sqrt(a.x * a.x + a.y * a.y);
}

double dvec_distance(dvec2 a, dvec2 b) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    return sqrt(dx * dx + dy * dy);
}

double dvec_angle(dvec2 a, dvec2 b) {
    double dot = a.x * b.x + a.y * b.y;
    if (dot < -1.0)
        dot = -1.0;
    else if (dot > 1.0)
        dot = 1.0;
    return acos(dot);
}