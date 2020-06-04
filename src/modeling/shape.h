#ifndef shape_h
#define shape_h

#include "element.h"
#include "constants.h"

#define SHAPE_FILL_POLY_MARGIN          0.1 // TODO: this is very sensitive, doesn't work if set to a smaller number
#define SHAPE_W_MIN                     0.1
#define SHAPE_W_MAX                     4.0
#define SHAPE_W_CIRCLE                  0.70710678


struct dvec;

struct Curve {
    double x, y;    /* curve start position, serial */
    double w;       /* rational Bezier weight, serial */
    double cx, cy;  /* control point (calculated) */
};

struct Shape {
    Curve curves[SHAPE_CURVES]; /* serial */
    Ids ids;
};

struct Former {
    Shape shape; /* serial */
    float x; /* serial */
};

Shape shape_make(double x, double y, double d1, double d2, double d3, double d4, double w1, double w2, double w3, double w4);
Shape shape_circle(double x, double y, double r);
Shape shape_ellipse(double x, double y, double rx, double ry);
Shape shape_oval(double x, double y, double rx, double lo_ry, double hi_ry);
Shape shape_rect(double x, double y, double w, double h);
Shape shape_right_semi_circle(double x, double y, double r);
Shape shape_symmetric(double x, double y, double rx, double lo_ry, double lo_w, double hi_ry, double hi_w);

void shape_inflate_curves(Curve *curves, double v);
void shape_copy_curves(Curve *src, Curve *dst, double dx, double dy, double margin);
void shape_copy_former(Former *src, Former *dst, double dx, double dy, double dz, double margin);
void shape_mirror_copy_curves(Curve *src, Curve *dst, double dx, double dy, double margin); // TODO: rename to shape_mirror_curves
void shape_mirror_former(Former *src, Former *dst, double dx, double dy, double dz, double margin);
void shape_update_curve_control_points(Curve *curves);

void shape_translate(Shape *shape, double dx, double dy);
void shape_get_vertices(Shape *shape, int count, dvec *verts);
dvec shape_bezier(double ax, double ay, double bx, double by, double cx, double cy, double t, double w);
dvec shape_centroid(Shape *shape);

#endif
