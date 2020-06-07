#include "math_vec.h"
#include <math.h>
#include <float.h>


/* :vec3 */

vec3::vec3() {}

vec3::vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

vec3::vec3(const vec2 &v, float _z) : x(v.x), y(v.y), z(_z) {}

vec3::vec3(float _x, const vec2 &v) : x(_x), y(v.x), z(v.y) {}

void vec3::set() {
    x = y = z = 0.0f;
}

void vec3::set(float _x, float _y, float _z) {
    x = _x;
    y = _y;
    z = _z;
}

vec3 &vec3::normalize() {
    float l = sqrt(x * x + y * y + z * z);
    x /= l;
    y /= l;
    z /= l;
    return *this;
}

float vec3::length() {
    return sqrt(x * x + y * y + z * z);
}

float vec3::dot(vec3 v) {
    return x * v.x + y * v.y + z * v.z;
}

vec3 vec3::cross(vec3 v) {
    return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}

vec3 vec3::operator-() {
    return vec3(-x, -y, -z);
}

vec3 vec3::operator+(vec3 v) {
    return vec3(x + v.x, y + v.y, z + v.z);
}

vec3 vec3::operator-(vec3 v) {
    return vec3(x - v.x, y - v.y, z - v.z);
}

vec3 vec3::operator*(float v) {
    return vec3(x * v, y * v, z * v);
}

vec3 vec3::operator/(float v) {
    return vec3(x / v, y / v, z / v);
}

vec3 &vec3::operator+=(vec3 v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

vec3 &vec3::operator-=(vec3 v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

vec3 &vec3::operator*=(float v) {
    x *= v;
    y *= v;
    z *= v;
    return *this;
}

vec3 &vec3::operator*=(vec3 v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
}

bool vec3::operator==(vec3 v) {
    return x == v.x && y == v.y && z == v.z;
}

bool vec3::operator!=(vec3 v) {
    return x != v.x || y != v.y || z != v.z;
}

/* :vec4 */

vec4::vec4() {}

vec4::vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

vec4::vec4(vec3 v, float _w) {
    x = v.x;
    y = v.y;
    z = v.z;
    w = _w;
}

/* :box3 */

box3::box3() {}

box3::box3(float minx, float miny, float minz, float maxx, float maxy, float maxz) : min(minx, miny, minz), max(maxx, maxy, maxz) {}

void box3::include(float x, float y, float z) {
    if (x < min.x)
        min.x = x;
    if (x > max.x)
        max.x = x;
    if (y < min.y)
        min.y = y;
    if (y > max.y)
        max.y = y;
    if (z < min.z)
        min.z = z;
    if (z > max.z)
        max.z = z;
}

void box3::include(vec3 v) {
    include(v.x, v.y, v.z);
}

void box3::include(box3 o) {
    include(o.min);
    include(o.max);
}

bool box3::intersects(box3 o) {
    return min.x <= o.max.x && max.x >= o.min.x &&
           min.y <= o.max.y && max.y >= o.min.y &&
           min.z <= o.max.z && max.z >= o.min.z;
}

box3 &box3::grow(vec3 v) {
    min -= v;
    max += v;
    return *this;
}

box3 &box3::grow(float x, float y, float z) {
    min.x -= x;
    min.y -= y;
    min.z -= z;
    max.x += x;
    max.y += y;
    max.z += z;
    return *this;
}

/* :ivec2 */

ivec2::ivec2() {}

ivec2::ivec2(int _i, int _j) : i(_i), j(_j) {}
