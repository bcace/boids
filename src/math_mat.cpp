#include "math_mat.h"
#include <math.h>


/* :mat3 */

void mat3::set_rotation(float a, vec3 u) {
    u.normalize();
    float c = cos(a), s = sin(a), _c = 1 - c;

    v[0][0] = c + u.x * u.x * _c;
    v[0][1] = u.y * u.x * _c + u.z * s;
    v[0][2] = u.z * u.x * _c - u.y * s;

    v[1][0] = u.x * u.y * _c - u.z * s;
    v[1][1] = c + u.y * u.y * _c;
    v[1][2] = u.z * u.y * _c + u.x * s;

    v[2][0] = u.x * u.z * _c + u.y * s;
    v[2][1] = u.y * u.z * _c - u.x * s;
    v[2][2] = c + u.z * u.z * _c;
}

vec3 mat3::operator*(vec3 o) {
    return vec3(v[0][0] * o.x + v[1][0] * o.y + v[2][0] * o.z,
                v[0][1] * o.x + v[1][1] * o.y + v[2][1] * o.z,
                v[0][2] * o.x + v[1][2] * o.y + v[2][2] * o.z);
}

/* :mat4 */

void mat4::set_identity() {
    v[0][0] = 1;
    v[0][1] = 0;
    v[0][2] = 0;
    v[0][3] = 0;

    v[1][0] = 0;
    v[1][1] = 1;
    v[1][2] = 0;
    v[1][3] = 0;

    v[2][0] = 0;
    v[2][1] = 0;
    v[2][2] = 1;
    v[2][3] = 0;

    v[3][0] = 0;
    v[3][1] = 0;
    v[3][2] = 0;
    v[3][3] = 1;
}

void mat4::scale(float f) {
    v[0][0] *= f;
    v[1][1] *= f;
    v[2][2] *= f;
}

void mat4::scale(vec3 o) {
    v[0][0] *= o.x;
    v[0][1] *= o.x;
    v[0][2] *= o.x;
    v[1][0] *= o.y;
    v[1][1] *= o.y;
    v[1][2] *= o.y;
    v[2][0] *= o.z;
    v[2][1] *= o.z;
    v[2][2] *= o.z;
}

void mat4::translate(vec3 o) {
    v[3][0] += o.x;
    v[3][1] += o.y;
    v[3][2] += o.z;
}

mat4 mat4::operator*(mat4 &m) {
    mat4 r;
    r.v[0][0] = v[0][0] * m.v[0][0] + v[1][0] * m.v[0][1] + v[2][0] * m.v[0][2] + v[3][0] * m.v[0][3];
    r.v[0][1] = v[0][1] * m.v[0][0] + v[1][1] * m.v[0][1] + v[2][1] * m.v[0][2] + v[3][1] * m.v[0][3];
    r.v[0][2] = v[0][2] * m.v[0][0] + v[1][2] * m.v[0][1] + v[2][2] * m.v[0][2] + v[3][2] * m.v[0][3];
    r.v[0][3] = v[0][3] * m.v[0][0] + v[1][3] * m.v[0][1] + v[2][3] * m.v[0][2] + v[3][3] * m.v[0][3];

    r.v[1][0] = v[0][0] * m.v[1][0] + v[1][0] * m.v[1][1] + v[2][0] * m.v[1][2] + v[3][0] * m.v[1][3];
    r.v[1][1] = v[0][1] * m.v[1][0] + v[1][1] * m.v[1][1] + v[2][1] * m.v[1][2] + v[3][1] * m.v[1][3];
    r.v[1][2] = v[0][2] * m.v[1][0] + v[1][2] * m.v[1][1] + v[2][2] * m.v[1][2] + v[3][2] * m.v[1][3];
    r.v[1][3] = v[0][3] * m.v[1][0] + v[1][3] * m.v[1][1] + v[2][3] * m.v[1][2] + v[3][3] * m.v[1][3];

    r.v[2][0] = v[0][0] * m.v[2][0] + v[1][0] * m.v[2][1] + v[2][0] * m.v[2][2] + v[3][0] * m.v[2][3];
    r.v[2][1] = v[0][1] * m.v[2][0] + v[1][1] * m.v[2][1] + v[2][1] * m.v[2][2] + v[3][1] * m.v[2][3];
    r.v[2][2] = v[0][2] * m.v[2][0] + v[1][2] * m.v[2][1] + v[2][2] * m.v[2][2] + v[3][2] * m.v[2][3];
    r.v[2][3] = v[0][3] * m.v[2][0] + v[1][3] * m.v[2][1] + v[2][3] * m.v[2][2] + v[3][3] * m.v[2][3];

    r.v[3][0] = v[0][0] * m.v[3][0] + v[1][0] * m.v[3][1] + v[2][0] * m.v[3][2] + v[3][0] * m.v[3][3];
    r.v[3][1] = v[0][1] * m.v[3][0] + v[1][1] * m.v[3][1] + v[2][1] * m.v[3][2] + v[3][1] * m.v[3][3];
    r.v[3][2] = v[0][2] * m.v[3][0] + v[1][2] * m.v[3][1] + v[2][2] * m.v[3][2] + v[3][2] * m.v[3][3];
    r.v[3][3] = v[0][3] * m.v[3][0] + v[1][3] * m.v[3][1] + v[2][3] * m.v[3][2] + v[3][3] * m.v[3][3];
    return r;
}

/* :mat4_stack */

mat4_stack::mat4_stack() : depth(0) {}

mat4 &mat4_stack::top() {
    return stack[depth];
}

mat4 &mat4_stack::push() {
    stack[depth + 1] = stack[depth];
    return stack[depth++];
}

void mat4_stack::pop() {
    if (depth) --depth;
}

void mat4_stack::set_identity() {
    stack[depth].set_identity();
}

void mat4_stack::scale(float f) {
    stack[depth].scale(f);
}

void mat4_stack::scale(vec3 o) {
    stack[depth].scale(o);
}

void mat4_stack::translate(vec3 o) {
    stack[depth].translate(o);
}
