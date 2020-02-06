#include "fuselage.h"
#include "object.h"
#include <math.h>


// TODO: document
vec3 _calculate_longitudinal_tangent_direction(vec3 pos, vec3 tail_pos, vec3 nose_pos) {
    vec3 tail_dir = (tail_pos - pos).normalize();
    vec3 nose_dir = (nose_pos - pos).normalize();

    float tail_angle = acos(-tail_dir.x);
    float nose_angle = acos(nose_dir.x);
    float sum_angle = tail_angle + nose_angle;

    if (sum_angle == 0.0)
        return vec3(1, 0, 0);

    return (nose_dir * tail_angle / sum_angle - tail_dir * nose_angle / sum_angle).normalize();
}

// TODO: move _loft_fuselage here and make this a part of its body
void fuselage_update_longitudinal_tangents(Fuselage *fuselage) {

    /* zero longitudinal tangents */

    for (int i = 0; i < fuselage->objects_count; ++i) {
        Objref *o_ref = fuselage->objects + i;
        for (int j = 0; j < SHAPE_CURVES; ++j) {
            o_ref->t_tangents[j] = vec3(0, 0, 0);
            o_ref->n_tangents[j] = vec3(0, 0, 0);
        }
    }

    /* calculate conn tangents */

    for (int i = 0; i < fuselage->conns_count; ++i) {
        Conn *conn = fuselage->conns + i;
        Objref *tail_o_ref = conn->tail_o;
        Objref *nose_o_ref = conn->nose_o;

        Former *tail_f = &tail_o_ref->n_skin_former;
        Former *tail_tail_f = &tail_o_ref->t_skin_former;
        Former *nose_f = &nose_o_ref->t_skin_former;
        Former *nose_nose_f = &nose_o_ref->n_skin_former;
        vec3 *t_tangents = tail_o_ref->n_tangents;
        vec3 *n_tangents = nose_o_ref->t_tangents;

        /* tail */

        for (int j = 0; j < SHAPE_CURVES; ++j) {
            t_tangents[j] += _calculate_longitudinal_tangent_direction(
                vec3(tail_f->x, tail_f->shape.curves[j].x, tail_f->shape.curves[j].y),
                vec3(tail_tail_f->x, tail_tail_f->shape.curves[j].x, tail_tail_f->shape.curves[j].y),
                vec3(nose_f->x, nose_f->shape.curves[j].x, nose_f->shape.curves[j].y)
            );
        }

        /* nose */

        for (int j = 0; j < SHAPE_CURVES; ++j) {
            n_tangents[j] += _calculate_longitudinal_tangent_direction(
                vec3(nose_f->x, nose_f->shape.curves[j].x, nose_f->shape.curves[j].y),
                vec3(tail_f->x, tail_f->shape.curves[j].x, tail_f->shape.curves[j].y),
                vec3(nose_nose_f->x, nose_nose_f->shape.curves[j].x, nose_nose_f->shape.curves[j].y)
            );
        }
    }

    /* normalize conn tangents and calculate opening tangents */

    for (int i = 0; i < fuselage->objects_count; ++i) {
        Objref *o_ref = fuselage->objects + i;
        Object *o = o_ref->object;
        Former *tail_f = &o_ref->t_skin_former;
        Former *nose_f = &o_ref->n_skin_former;

        /* tail */

        if (o_ref->t_conns_count == 0) { /* opening tangents */

            for (int j = 0; j < SHAPE_CURVES; ++j) {
                o_ref->t_tangents[j] = _calculate_longitudinal_tangent_direction(
                    vec3(tail_f->x, tail_f->shape.curves[j].x, tail_f->shape.curves[j].y),
                    vec3(tail_f->x - o->tail_endp_dx, o_ref->y, o_ref->z),
                    vec3(nose_f->x, nose_f->shape.curves[j].x, nose_f->shape.curves[j].y)
                );
            }
        }
        else { /* conn tangents */

            for (int j = 0; j < SHAPE_CURVES; ++j) {
                vec3 *v = o_ref->t_tangents + j;
                float l = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
                v->x /= l;
                v->y /= l;
                v->z /= l;
            }
        }

        /* nose */

        if (o_ref->n_conns_count == 0) { /* opening tangents */

            for (int j = 0; j < SHAPE_CURVES; ++j) {
                o_ref->n_tangents[j] = _calculate_longitudinal_tangent_direction(
                    vec3(nose_f->x, nose_f->shape.curves[j].x, nose_f->shape.curves[j].y),
                    vec3(tail_f->x, tail_f->shape.curves[j].x, tail_f->shape.curves[j].y),
                    vec3(nose_f->x + o->nose_endp_dx, o_ref->y, o_ref->z)
                );
            }
        }
        else { /* conn tangents */

            for (int j = 0; j < SHAPE_CURVES; ++j) {
                vec3 *v = o_ref->n_tangents + j;
                float l = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
                v->x /= l;
                v->y /= l;
                v->z /= l;
            }
        }
    }
}
