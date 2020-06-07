#include "modeling_collision.h"
#include "modeling_shape.h"
#include <math.h>
#include <float.h>
#include <assert.h>


void coll_get_prism(Shape *s, CollPrism *prism, double dx, double dy, double margin) {
    Shape transformed_s;
    shape_copy_curves(s->curves, transformed_s.curves, dx, dy, margin);
    shape_get_vertices(&transformed_s, COLL_PRISM_VERTS, prism->verts);
}

bool coll_interact_prisms(CollPrism *prism1, CollPrism *prism2, double *f) {
    f[0] = f[1] = f[2] = 0.0;

    if (prism1->min_x > prism2->max_x || prism1->max_x < prism2->min_x) // prisms don't overlap along x
        return false;

    struct Isect {
        double x, y;
        int indices[2]; // node indices on respective curves
        bool is_start;
    } isects[32];
    int isects_count = 0;

    // find all intersection points

    for (int i = 0; i < COLL_PRISM_VERTS; ++i) {
        int next_i = (i + 1) % COLL_PRISM_VERTS;

        dvec n11 = prism1->verts[i];
        dvec n12 = prism1->verts[next_i];

        double _dx1 = n11.x - n12.x;
        double _dy1 = n11.y - n12.y;

        for (int j = 0; j < COLL_PRISM_VERTS; ++j) {
            int next_j = (j + 1) % COLL_PRISM_VERTS;

            dvec n21 = prism2->verts[j];
            dvec n22 = prism2->verts[next_j];

            double _dx2 = n21.x - n22.x;
            double _dy2 = n21.y - n22.y;

            double det = _dx1 * _dy2 - _dy1 * _dx2;
            if (fabs(det) < 0.0001) // lines are parallel or coincident
                continue;

            double _dx12 = n11.x - n21.x;
            double _dy12 = n11.y - n21.y;

            double t =  (_dx12 * _dy2 - _dy12 * _dx2) / det;
            if (t < 0.0 || t >= 1.0) // intersection outside first line segment
                continue;

            double u = -(_dx1 * _dy12 - _dy1 * _dx12) / det;
            if (u < 0.0 || u >= 1.0) // intersection outside second line segment
                continue;

            double x = n11.x - t * _dx1;
            double y = n11.y - t * _dy1;

            Isect &isect = isects[isects_count++];
            isect.x = x;
            isect.y = y;
            isect.indices[0] = next_i;
            isect.indices[1] = next_j;
            isect.is_start = (-_dx2 * _dy1 + -_dy2 * -_dx1) < 0.0;
        }
    }

    if (isects_count == 0)
        return false;

    if (isects_count % 2 != 0)
        return false;

    double min_f_magn = DBL_MAX;
    double min_f_isect_norm_x;
    double min_f_isect_norm_y;

    for (int i = 0; i < isects_count; ++i) {
        int next_i = (i + 1) % isects_count;

        Isect &isect = isects[i];
        Isect &next_isect = isects[next_i];

        if (!isect.is_start)
            continue;

        assert(next_isect.is_start == false);

        double max[2] = { 0.0, 0.0 };

        double isect_line_x = next_isect.x - isect.x;
        double isect_line_y = next_isect.y - isect.y;
        double isect_length = sqrt(isect_line_x * isect_line_x + isect_line_y * isect_line_y);
        double isect_norm_x = -isect_line_y / isect_length;
        double isect_norm_y =  isect_line_x / isect_length;

        for (int k = isect.indices[0]; k != next_isect.indices[0]; k = (k + 1) % COLL_PRISM_VERTS) {
            double dx = prism1->verts[k].x - isect.x;
            double dy = prism1->verts[k].y - isect.y;
            double d = dx * isect_norm_x + dy * isect_norm_y;
            if (d > max[0])
                max[0] = d;
        }

        for (int k = next_isect.indices[1]; k != isect.indices[1]; k = (k + 1) % COLL_PRISM_VERTS) {
            double dx = prism2->verts[k].x - isect.x;
            double dy = prism2->verts[k].y - isect.y;
            double d = dx * isect_norm_x + dy * isect_norm_y;
            if (d > max[1])
                max[1] = d;
        }

        double f_magn = (max[0] + max[1]) * 0.5;
        if (f_magn < min_f_magn) {
            min_f_magn = f_magn;
            min_f_isect_norm_x = isect_norm_x;
            min_f_isect_norm_y = isect_norm_y;
        }
    }

    f[1] = min_f_isect_norm_x * -min_f_magn;
    f[2] = min_f_isect_norm_y * -min_f_magn;

    return true;
}
