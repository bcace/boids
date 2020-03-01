#ifndef interp_h
#define interp_h


struct vec2;
struct vec3;
struct dvec;
struct tvec;

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
tvec cube_bezier_3d(tvec e1, tvec c1, tvec c2, tvec e2, double t);

float quad_bezier_1f(float a, float b, float c, float t);
double quad_bezier_1d(double a, double b, double c, double t);
vec2 quad_bezier_2f(vec2 a, vec2 b, vec2 c, float t);
vec2 quad_bezier_2f_derivative(vec2 a, vec2 b, vec2 c, float t);
float quad_bezier_2f_param_at_derivative(vec2 a, vec2 b, vec2 c, float deriv);

#define SMOOTHSTEP(t) ((t) * (t) * (t) * ((t) * ((t) * 6 - 15) + 10))

#endif
