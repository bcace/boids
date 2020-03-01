#include "dvec.h"
#include <math.h>


tvec tvec_init(double x, double y, double z) {
    tvec v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

tvec tvec_zero() {
    tvec v;
    v.x = v.y = v.z = 0.0;
    return v;
}

tvec tvec_norm(tvec v) {
    double l = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x /= l;
    v.y /= l;
    v.z /= l;
    return v;
}

tvec tvec_scale(tvec v, double s) {
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return v;
}

tvec tvec_add(tvec a, tvec b) {
    tvec v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    return v;
}

tvec tvec_sub(tvec a, tvec b) {
    tvec v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    return v;
}
