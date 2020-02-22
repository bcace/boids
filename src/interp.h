#ifndef interp_h
#define interp_h


double lerp_1d(double a, double b, double t);
double lerp_angle_1d(double a, double b, double t);
float lerp_1f(float a, float b, float t);
vec2 lerp_2f(vec2 a, vec2 b, float t);
vec3 lerp_3f(vec3 a, vec3 b, float t);

vec2 bezier(vec2 e1, vec2 c1, vec2 c2, vec2 e2, float t);
vec3 bezier(vec3 e1, vec3 c1, vec3 c2, vec3 e2, float t);
vec2 split_bezier(vec2 e1, vec2 c1, vec2 c2, vec2 e2, float t,
                  vec2 &c11,    /* replacement control point for c1 */
                  vec2 &c12,    /* control point before split point */
                  vec2 &c21,    /* control point after split point */
                  vec2 &c22     /* replacement control point for c2 */
                  );
vec3 split_bezier(vec3 e1, vec3 c1, vec3 c2, vec3 e2, float t,
                  vec3 &c11,    /* replacement control point for c1 */
                  vec3 &c12,    /* control point before split point */
                  vec3 &c21,    /* control point after split point */
                  vec3 &c22     /* replacement control point for c2 */
                  );

float cube_bezier_1f(float e1, float c1, float c2, float e2, float t);

float quad_bezier_1f(float a, float b, float c, float t);
double quad_bezier_1d(double a, double b, double c, double t);
vec2 quad_bezier_2f(vec2 a, vec2 b, vec2 c, float t);
vec2 quad_bezier_2f_derivative(vec2 a, vec2 b, vec2 c, float t);
float quad_bezier_2f_param_at_derivative(vec2 a, vec2 b, vec2 c, float deriv);

#define SMOOTHSTEP(t) ((t) * (t) * (t) * ((t) * ((t) * 6 - 15) + 10))

#endif

/*
** implementation
*/

#ifdef INTERP_IMPL
#undef INTERP_IMPL

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

vec2 lerp_2f(vec2 a, vec2 b, float t) {
    return a + (b - a) * t;
}

vec3 lerp_3f(vec3 a, vec3 b, float t) {
    return a + (b - a) * t;
}

vec2 bezier(vec2 e1, vec2 c1, vec2 c2, vec2 e2, float t) {
    double _t = 1.0 - t;
    double a = _t * _t * _t;
    double b = _t * _t * t * 3.0;
    double c = _t * t * t * 3.0;
    double d = t * t * t;
    return vec2(e1.x * a + c1.x * b + c2.x * c + e2.x * d,
                e1.y * a + c1.y * b + c2.y * c + e2.y * d);
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

vec2 split_bezier(vec2 e1, vec2 c1, vec2 c2, vec2 e2,
                  float t,
                  vec2 &c11, vec2 &c12, vec2 &c21, vec2 &c22) {
    float _t = 1.0 - t;
    c11 = e1 * _t + c1 * t;
    c22 = e2 * t + c2 * _t;
    vec2 c = c1 * _t + c2 * t;
    c12 = c11 * _t + c * t;
    c21 = c22 *t + c * _t;
    return c12 * _t + c21 * t;
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

double quad_bezier_1d(double a, double b, double c, double t) {
    double _t = 1.0 - t;
    return (a * _t * _t) + (b * t * _t * 2.0) + (c * t * t);
}

float quad_bezier_1f(float a, float b, float c, float t) {
    float _t = 1.0f - t;
    return (a * _t * _t) + (b * t * _t * 2.0f) + (c * t * t);
}

vec2 quad_bezier_2f(vec2 a, vec2 b, vec2 c, float t) {
    float _t = 1.0f - t;
    return (a * _t * _t) + (b * t * _t * 2.0f) + (c * t * t);
}

vec2 quad_bezier_2f_derivative(vec2 a, vec2 b, vec2 c, float t) {
    return ((b - a) * (1.0f - t) + (c - b) * t) * 2.0f;
}

float quad_bezier_2f_param_at_derivative(vec2 a, vec2 b, vec2 c, float deriv) {
    return (a.y - b.y - (a.x - b.x) * deriv) / (a.y + c.y - 2 * b.y - (a.x + c.x - 2 * b.x) * deriv);
}

#endif
