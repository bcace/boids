#include "modeling_shape.h"
#include "memory_arena.h"
#include "math_dvec.h"
#include <assert.h>


Shape shape_make(double x, double y, double d1, double d2, double d3, double d4, double w1, double w2, double w3, double w4) {
    Shape s;
    Curve *curves = s.curves;

    curves[0].x = x + d1;
    curves[0].y = y;
    curves[1].x = x;
    curves[1].y = y + d2;
    curves[2].x = x - d3;
    curves[2].y = y;
    curves[3].x = x;
    curves[3].y = y - d4;

    assert(w1 >= SHAPE_W_MIN && w1 <= SHAPE_W_MAX);
    assert(w2 >= SHAPE_W_MIN && w2 <= SHAPE_W_MAX);
    assert(w3 >= SHAPE_W_MIN && w3 <= SHAPE_W_MAX);
    assert(w4 >= SHAPE_W_MIN && w4 <= SHAPE_W_MAX);

    curves[0].w = w1;
    curves[1].w = w2;
    curves[2].w = w3;
    curves[3].w = w4;

    shape_update_curve_control_points(curves);

    return s;
}

Shape shape_circle(double x, double y, double r) {
    return shape_make(x, y, r, r, r, r, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE);
}

Shape shape_ellipse(double x, double y, double rx, double ry) {
    return shape_make(x, y, rx, ry, rx, ry, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE);
}

Shape shape_oval(double x, double y, double rx, double lo_ry, double hi_ry) {
    return shape_make(x, y, rx, hi_ry, rx, lo_ry, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE, SHAPE_W_CIRCLE);
}

Shape shape_rect(double x, double y, double w, double h) {
    return shape_make(x, y, w * 0.5, h * 0.5, w * 0.5, h * 0.5, SHAPE_W_MAX, SHAPE_W_MAX, SHAPE_W_MAX, SHAPE_W_MAX);
}

Shape shape_right_semi_circle(double x, double y, double r) {
    return shape_make(x, y, r, r, r * 0.1, r, SHAPE_W_CIRCLE, SHAPE_W_MAX, SHAPE_W_MAX, SHAPE_W_CIRCLE);
}

Shape shape_symmetric(double x, double y, double rx, double lo_ry, double lo_w, double hi_ry, double hi_w) {
    return shape_make(x, y, rx, hi_ry, rx, lo_ry, hi_w, hi_w, lo_w, lo_w);
}

void shape_inflate_curves(Curve *curves, double v) {
    curves[0].x += v;
    curves[1].y += v;
    curves[2].x -= v;
    curves[3].y -= v;
    shape_update_curve_control_points(curves);
}

void shape_copy_curves(Curve *src, Curve *dst, double dx, double dy, double margin) {
    dst[0].x = src[0].x + dx + margin;
    dst[0].y = src[0].y + dy;
    dst[1].x = src[1].x + dx;
    dst[1].y = src[1].y + dy + margin;
    dst[2].x = src[2].x + dx - margin;
    dst[2].y = src[2].y + dy;
    dst[3].x = src[3].x + dx;
    dst[3].y = src[3].y + dy - margin;
    dst[0].w = src[0].w;
    dst[1].w = src[1].w;
    dst[2].w = src[2].w;
    dst[3].w = src[3].w;
    shape_update_curve_control_points(dst);
}

void shape_copy_former(Former *src, Former *dst, double dx, double dy, double dz, double margin) {
    dst->x = src->x + dx;
    shape_copy_curves(src->shape.curves, dst->shape.curves, dy, dz, margin);
}

void shape_mirror_copy_curves(Curve *src, Curve *dst, double dx, double dy, double margin) {
    dst[0].x = -src[2].x + dx + margin;
    dst[0].y =  src[2].y + dy;
    dst[1].x = -src[1].x + dx;
    dst[1].y =  src[1].y + dy + margin;
    dst[2].x = -src[0].x + dx - margin;
    dst[2].y =  src[0].y + dy;
    dst[3].x = -src[3].x + dx;
    dst[3].y =  src[3].y + dy - margin;
    dst[0].w =  src[1].w;
    dst[1].w =  src[0].w;
    dst[2].w =  src[3].w;
    dst[3].w =  src[2].w;
    shape_update_curve_control_points(dst);
}

void shape_mirror_former(Former *src, Former *dst, double dx, double dy, double dz, double margin) {
    dst->x = src->x + dx;
    shape_mirror_copy_curves(src->shape.curves, dst->shape.curves, dy, dz, margin);
}

void shape_update_curve_control_points(Curve *curves) {
    curves[0].cx = curves[0].x;
    curves[0].cy = curves[1].y;
    curves[1].cx = curves[2].x;
    curves[1].cy = curves[1].y;
    curves[2].cx = curves[2].x;
    curves[2].cy = curves[3].y;
    curves[3].cx = curves[0].x;
    curves[3].cy = curves[3].y;
}

void shape_translate(Shape *shape, double dx, double dy) {
    for (int i = 0; i < SHAPE_CURVES; ++i) {
        shape->curves[i].x += dx;
        shape->curves[i].y += dy;
        shape->curves[i].cx += dx;
        shape->curves[i].cy += dy;
    }
}

dvec shape_bezier(double ax, double ay, double bx, double by, double cx, double cy, double t, double w) {
    double _t = 1.0 - t;
    double _a = _t * _t;
    double _b = 2.0f * t * _t * w;
    double _c = t * t;
    dvec v;
    v.x = (ax * _a + bx * _b + cx * _c) / (_a + _b + _c);
    v.y = (ay * _a + by * _b + cy * _c) / (_a + _b + _c);
    return v;
}

void shape_get_vertices(Shape *s, int count, dvec *verts) {
    assert(count % SHAPE_CURVES == 0);
    int curve_subdivs = count / SHAPE_CURVES;
    double dt = 1.0 / curve_subdivs;
    dvec *v = verts;
    for (int i = 0; i < SHAPE_CURVES; ++i) {
        int next_i = (i + 1) % SHAPE_CURVES;
        for (int j = 0; j < curve_subdivs; ++j) {
            double t = j * dt;
            *v++ = shape_bezier(
                s->curves[i].x, s->curves[i].y,
                s->curves[i].cx, s->curves[i].cy,
                s->curves[next_i].x, s->curves[next_i].y,
                t, s->curves[i].w
            );
        }
    }
}

dvec shape_centroid(Shape *shape) {
    dvec v;
    v.x = shape->curves[1].x;
    v.y = shape->curves[0].y;
    return v;
}
