#ifndef dvec_h
#define dvec_h


// TODO: why do we have this?
struct dvec {
    double x, y;
};

struct dvec2 {
    double x, y;
};

dvec2 dvec_init(double x, double y);
dvec2 dvec_plus(dvec2 a, dvec2 b);
dvec2 dvec_minus(dvec2 a, dvec2 b);
dvec2 dvec_divide(dvec2 a, double b);
double dvec_dot(dvec2 a, dvec2 b);
double dvec_cross(dvec2 a, dvec2 b);
double dvec_length(dvec2 a);
double dvec_distance(dvec2 a, dvec2 b);
double dvec_angle(dvec2 a, dvec2 b); /* assumes both vectors are normalized */

#endif
