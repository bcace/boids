#ifndef mat_h
#define mat_h

#include "vec.h"


struct mat3 {
    float v[3][3];

    void set_rotation(float a, vec3 u);
    vec3 operator*(vec3 o);
};

struct mat4 {
    float v[4][4];

    void set_identity();
    void scale(float f);
    void scale(vec3 o);
    void translate(vec3 o);
    mat4 operator*(mat4 &m);
};

struct mat4_stack {
    mat4 stack[32];
    int depth;

    mat4_stack();

    mat4 &top();
    mat4 &push();
    void pop();

    void set_identity();
    void scale(float f);
    void scale(vec3 o);
    void translate(vec3 o);
};

#endif
