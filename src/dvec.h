#ifndef dvec_h
#define dvec_h


struct dvec {
    double x, y;
};

dvec dvec_rotate(dvec v, double a);

struct tvec {
    double x, y, z;
};

tvec tvec_init(double x, double y, double z);
tvec tvec_zero();
tvec tvec_norm(tvec v);
tvec tvec_scale(tvec v, double s);
tvec tvec_add(tvec a, tvec b);
tvec tvec_sub(tvec a, tvec b);

#endif
