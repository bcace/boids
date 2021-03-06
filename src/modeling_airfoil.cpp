#include "modeling_airfoil.h"
#include "math_dvec.h"
#include "math_vec.h"
#include <math.h>
#include <string.h>


const double _AIRFOIL_DT = 1.0 / AIRFOIL_X_SUBDIVS;

double airfoil_get_subdiv_x(int i) {
    double t = (i + 1) * _AIRFOIL_DT;
    return t * t;
}

dvec airfoil_get_point(Airfoil *airfoil, int i) {
    dvec p;

    if (i < AIRFOIL_X_SUBDIVS) {
        AirfoilSide *side = &airfoil->upper;
        int j = AIRFOIL_X_SUBDIVS - i - 1;
        p.x = airfoil_get_subdiv_x(j);
        p.y = side->base + side->y[j] * side->delta;
    }
    else if (i == AIRFOIL_X_SUBDIVS) {
        p.x = 0.0;
        p.y = 0.0;
    }
    else {
        AirfoilSide *side = &airfoil->lower;
        int j = i - AIRFOIL_X_SUBDIVS - 1;
        p.x = airfoil_get_subdiv_x(j);
        p.y = side->base + side->y[j] * side->delta;
    }

    return p;
}

void airfoil_init(Airfoil *airfoil, const char *name,
                  float u_base, float u_delta, unsigned char *u_y,
                  float l_base, float l_delta, unsigned char *l_y) {
    strcpy_s(airfoil->name, name);

    memcpy(airfoil->upper.y, u_y, sizeof(unsigned char) * AIRFOIL_X_SUBDIVS);
    airfoil->upper.base = u_base;
    airfoil->upper.delta = u_delta;

    memcpy(airfoil->lower.y, l_y, sizeof(unsigned char) * AIRFOIL_X_SUBDIVS);
    airfoil->lower.base = l_base;
    airfoil->lower.delta = l_delta;
}

float _get_side_trailing_offset(AirfoilSide *s) {
    return s->base + s->y[AIRFOIL_X_SUBDIVS - 1] * s->delta;
}

float airfoil_get_trailing_y_offset(Airfoil *a) {
    return (_get_side_trailing_offset(&a->upper) +
            _get_side_trailing_offset(&a->lower)) * 0.5f;
}

void airfoil_get_drawing_verts(Airfoil *airfoil, vec3 *verts, double dihedral, double chord, double aoa, double x, double y, double z) {

    AirfoilSide *u_side = &airfoil->upper;
    AirfoilSide *l_side = &airfoil->lower;
    vec3 *v = verts;

    for (int i = AIRFOIL_X_SUBDIVS - 1; i >= 0; --i) {
        v->x = -airfoil_get_subdiv_x(i);
        v->y = 0.0;
        v->z = u_side->base + u_side->y[i] * u_side->delta;
        ++v;
    }

    v->x = 0.0;
    v->y = 0.0;
    v->z = 0.0;
    ++v;

    for (int i = 0; i < AIRFOIL_X_SUBDIVS; ++i) {
        v->x = -airfoil_get_subdiv_x(i);
        v->y = 0.0;
        v->z = l_side->base + l_side->y[i] * l_side->delta;
        ++v;
    }

    double cos_b = cos(-aoa);
    double sin_b = sin(-aoa);
    double cos_g = cos(dihedral);
    double sin_g = sin(dihedral);

    for (int i = 0; i < AIRFOIL_POINTS; ++i) { /* transform vertices */
        vec3 o = verts[i];
        verts[i].x = x + (o.x * cos_b + o.y * sin_b * sin_g + o.z * sin_b * cos_g) * chord;
        verts[i].y = y + (o.y * cos_g - o.z * sin_g) * chord;
        verts[i].z = z + (-o.x * sin_b + o.y * cos_b * sin_g + o.z * cos_b * cos_g) * chord;
    }
}

tvec _transform_vertex(double x, double y, double z, double c, double cb, double sb, double cg, double sg, double dx, double dy, double dz) {
    tvec v;
    v.x = dx + (x * cb + y * sb * sg + z * sb * cg) * c;
    v.y = dy + (y * cg - z * sg) * c;
    v.z = dz + (-x * sb + y * cb * sg + z * cb * cg) * c;
    return v;
}

void airfoil_get_verts(Airfoil *a, tvec *u_verts, tvec *l_verts, double dihedral, double chord, double aoa, double dx, double dy, double dz) {
    AirfoilSide *u_side = &a->upper;
    AirfoilSide *l_side = &a->lower;

    double cb = cos(-aoa);
    double sb = sin(-aoa);
    double cg = cos(dihedral);
    double sg = sin(dihedral);

    tvec *u_vert = u_verts;
    tvec *l_vert = l_verts;
    for (int i = AIRFOIL_X_SUBDIVS - 1; i >= 0; --i) {
        *u_vert++ = _transform_vertex(-airfoil_get_subdiv_x(i),
                                      0.0,
                                      u_side->base + u_side->y[i] * u_side->delta,
                                      chord,
                                      cb, sb, cg, sg,
                                      dx, dy, dz);
        *l_vert++ = _transform_vertex(-airfoil_get_subdiv_x(i),
                                      0.0,
                                      l_side->base + l_side->y[i] * l_side->delta,
                                      chord,
                                      cb, sb, cg, sg,
                                      dx, dy, dz);
    }
    *u_vert = *l_vert = _transform_vertex(0.0,
                                          0.0,
                                          0.0,
                                          chord,
                                          cb, sb, cg, sg,
                                          dx, dy, dz);
}
