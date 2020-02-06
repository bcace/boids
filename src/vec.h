#ifndef vec_h
#define vec_h


struct vec2 {
    union {
        struct {
            float x, y;
        };
        struct {
            float a, b;
        };
        float v[2];
    };

    vec2();
    vec2(float _x, float _y);

    void set(float _x, float _y);
    vec2 &normalize();
    void normalize_to(float v);
    float length();
    float angle_to(vec2 v); /* [0, pi], assumes both vectors are normalized */
    float dot(vec2 v);
    float cross(vec2 v);
    vec2 operator-();
    vec2 operator+(vec2 v);
    vec2 operator-(vec2 v);
    vec2 operator*(float v);
    vec2 operator/(float v);
    vec2 operator*(vec2 v);
    vec2 &operator+=(vec2 v);
    vec2 &operator-=(vec2 v);
    vec2 &operator/=(float v);
    vec2 &operator*=(float v);
    bool operator!=(vec2 v);
    vec2 rot(float a);
    vec2 perp_positive();
    vec2 perp_negative();
    void print(const char *tag="");
};

struct vec3 {
    union {
        struct {
            float x, y, z;
        };
        struct {
            float r, g, b;
        };
        float v[3];
    };

    vec3();
    vec3(float x, float y, float z);
    vec3(const struct vec2 &v, float z);
    vec3(float x, const struct vec2 &v);

    void set();
    void set(float _x, float _y, float _z);
    vec3 &normalize();
    float length();
    float angle_to(vec3 v); /* [0, pi], assumes both vectors are normalized */
    float dot(vec3 v);
    vec3 cross(vec3 v);
    vec3 operator-();
    vec3 operator+(vec3 v);
    vec3 operator-(vec3 v);
    vec3 operator*(float v);
    vec3 operator/(float v);
    vec3 &operator+=(vec3 v);
    vec3 &operator-=(vec3 v);
    vec3 &operator*=(float v);
    vec3 &operator*=(vec3 v);
    bool operator==(vec3 v);
    bool operator!=(vec3 v);
    void print(const char *tag="");
    vec2 yz();
};

struct vec4 {
    union {
        struct {
            float x, y, z, w;
        };
        struct {
            float r, g, b, a;
        };
        float v[4];
    };

    vec4();
    vec4(float _x, float _y, float _z, float _w);
    vec4(vec3 v, float _w);
    void print(const char *tag="");
};

struct box2 {
    vec2 min, max;

    box2();
    box2(float minx, float miny, float maxx, float maxy);

    void reset();
    void include(vec2 v);
    void include(double x, double y);
    bool intersects(box2 o);
};

struct box3 {
    vec3 min, max;

    box3();
    box3(float minx, float miny, float minz, float maxx, float maxy, float maxz);

    void include(float x, float y, float z);
    void include(vec3 v);
    void include(box3 o);
    bool intersects(box3 o);
    box3 &grow(vec3 v);
    box3 &grow(float x, float y, float z);
};

struct ivec2 {
    union {
        struct {
            int x, y;
        };
        struct {
            int i, j;
        };
        int v[2];
    };

    ivec2();
    ivec2(int _i, int _j);
};

#endif

/*
** implementation
*/

#ifdef VEC_IMPL
#undef VEC_IMPL

#include <math.h>
#include <float.h>
#include <stdio.h>

/* :vec2 */

vec2::vec2() {}

vec2::vec2(float _x, float _y) : x(_x), y(_y) {}

void vec2::set(float _x, float _y) {
    x = _x;
    y = _y;
}

vec2 vec2::rot(float a) {
    if (a == 0.0)
        return *this;
    double c = cos(a);
    double s = sin(a);
    return vec2(x * c - y * s, x * s + y * c);
}

vec2 &vec2::normalize() {
    float l = sqrt(x * x + y * y);
    x /= l;
    y /= l;
    return *this;
}

void vec2::normalize_to(float v) {
    float l = v / sqrt(x * x + y * y);
    x *= l;
    y *= l;
}

float vec2::length() {
    return sqrt(x * x + y * y);
}

float vec2::angle_to(vec2 v) {
    float d = dot(v);
    if (d < -1.0)
        d = -1.0;
    else if (d > 1.0)
        d = 1.0;
    return acos(d);
}

float vec2::dot(vec2 v) {
    return x * v.x + y * v.y;
}

float vec2::cross(vec2 v) {
    return x * v.y - y * v.x;
}

vec2 vec2::operator-() {
    return vec2(-x, -y);
}

vec2 vec2::operator+(vec2 v) {
    return vec2(x + v.x, y + v.y);
}

vec2 vec2::operator-(vec2 v) {
    return vec2(x - v.x, y - v.y);
}

vec2 vec2::operator*(float v) {
    return vec2(x * v, y * v);
}

vec2 vec2::operator/(float v) {
    return vec2(x / v, y / v);
}

vec2 vec2::operator*(vec2 v) {
    return vec2(x * v.x, y * v.y);
}

vec2 &vec2::operator+=(vec2 v) {
    x += v.x;
    y += v.y;
    return *this;
}

vec2 &vec2::operator-=(vec2 v) {
    x -= v.x;
    y -= v.y;
    return *this;
}

vec2 &vec2::operator*=(float v) {
    x *= v;
    y *= v;
    return *this;
}

vec2 &vec2::operator/=(float v) {
    x /= v;
    y /= v;
    return *this;
}

bool vec2::operator!=(vec2 v) {
    return x != v.x || y != v.y;
}

vec2 vec2::perp_positive() {
    return vec2(-y, x);
}

vec2 vec2::perp_negative() {
    return vec2(y, -x);
}

void vec2::print(const char *tag) {
    fprintf(stderr, "%s %g, %g\n", tag, x, y);
}

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

float vec3::angle_to(vec3 v) {
    float d = dot(v);
    if (d < -1.0)
        d = -1.0;
    else if (d > 1.0)
        d = 1.0;
    return acos(d);
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

void vec3::print(const char *tag) {
    fprintf(stderr, "%s %g, %g, %g\n", tag, x, y, z);
}

vec2 vec3::yz() {
    return vec2(y, z);
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

void vec4::print(const char *tag) {
    fprintf(stderr, "%s %g, %g, %g, %g\n", tag, x, y, z, w);
}

/* :box2 */

box2::box2() {}

box2::box2(float minx, float miny, float maxx, float maxy) : min(minx, miny), max(maxx, maxy) {}

void box2::reset() {
    min.x = min.y = FLT_MAX;
    max.x = max.y = -FLT_MAX;
}

void box2::include(vec2 v) {
    if (v.x < min.x)
        min.x = v.x;
    if (v.x > max.x)
        max.x = v.x;
    if (v.y < min.y)
        min.y = v.y;
    if (v.y > max.y)
        max.y = v.y;
}

void box2::include(double x, double y) {
    if (x < min.x)
        min.x = x;
    if (x > max.x)
        max.x = x;
    if (y < min.y)
        min.y = y;
    if (y > max.y)
        max.y = y;
}

bool box2::intersects(box2 o) {
    return min.x <= o.max.x && max.x >= o.min.x &&
           min.y <= o.max.y && max.y >= o.min.y;
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

#endif
