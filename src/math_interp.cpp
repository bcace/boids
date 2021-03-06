#include "math_interp.h"
#include "math_vec.h"
#include "math_dvec.h"

#define _INTERP_PI  3.141592654
#define _INTERP_PI2 6.283185307


double lerp_1d(double a, double b, double t) {
    return a + (b - a) * t;
}

double lerp_angle_1d(double a, double b, double t) {
    if (b - a > _INTERP_PI)
        a += _INTERP_PI2;
    else if (a - b > _INTERP_PI)
        b += _INTERP_PI2;
    return a + (b - a) * t;
}

float lerp_1f(float a, float b, float t) {
    return a + (b - a) * t;
}

vec3 lerp_3f(vec3 a, vec3 b, float t) {
    return a + (b - a) * t;
}

vec3 bezier(vec3 e1, vec3 c1, vec3 c2, vec3 e2, float t) {
    double _t = 1.0 - t;
    double a = _t * _t * _t;
    double b = _t * _t * t * 3.0;
    double c = _t * t * t * 3.0;
    double d = t * t * t;
    return vec3(e1.x * a + c1.x * b + c2.x * c + e2.x * d,
                e1.y * a + c1.y * b + c2.y * c + e2.y * d,
                e1.z * a + c1.z * b + c2.z * c + e2.z * d);
}

vec3 split_bezier(vec3 e1, vec3 c1, vec3 c2, vec3 e2,
                  float t,
                  vec3 &c11, vec3 &c12, vec3 &c21, vec3 &c22) {
    float _t = 1.0 - t;
    c11 = e1 * _t + c1 * t;
    c22 = e2 * t + c2 * _t;
    vec3 c = c1 * _t + c2 * t;
    c12 = c11 * _t + c * t;
    c21 = c22 *t + c * _t;
    return c12 * _t + c21 * t;
}

float cube_bezier_1f(float e1, float c1, float c2, float e2, float t) {
    double _t = 1.0 - t;
    double a = _t * _t * _t;
    double b = _t * _t * t * 3.0;
    double c = _t * t * t * 3.0;
    double d = t * t * t;
    return e1 * a + c1 * b + c2 * c + e2 * d;
}

tvec cube_bezier_3d(tvec e1, tvec c1, tvec c2, tvec e2, double t) {
    double _t = 1.0 - t;
    double a = _t * _t * _t;
    double b = _t * _t * t * 3.0;
    double c = _t * t * t * 3.0;
    double d = t * t * t;
    tvec r;
    r.x = e1.x * a + c1.x * b + c2.x * c + e2.x * d;
    r.y = e1.y * a + c1.y * b + c2.y * c + e2.y * d;
    r.z = e1.z * a + c1.z * b + c2.z * c + e2.z * d;
    return r;
}

double quad_bezier_1d(double a, double b, double c, double t) {
    double _t = 1.0 - t;
    return (a * _t * _t) + (b * t * _t * 2.0) + (c * t * t);
}

float quad_bezier_1f(float a, float b, float c, float t) {
    float _t = 1.0f - t;
    return (a * _t * _t) + (b * t * _t * 2.0f) + (c * t * t);
}
