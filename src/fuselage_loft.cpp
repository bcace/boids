#include "fuselage.h"
#include "object.h"
#include "interp.h"
#include "mesh.h"
#include "arena.h"
#include <math.h>
#include <float.h>


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

void _update_longitudinal_tangents(Fuselage *fuselage) {

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
        Objref *t_oref = conn->tail_o;
        Objref *n_oref = conn->nose_o;

        Former *tail_f = &t_oref->n_skin_former;
        Former *tail_tail_f = &t_oref->t_skin_former;
        Former *nose_f = &n_oref->t_skin_former;
        Former *nose_nose_f = &n_oref->n_skin_former;
        vec3 *t_tangents = t_oref->n_tangents;
        vec3 *n_tangents = n_oref->t_tangents;

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

/* Returns a shape that represents a section of object or connection. */
static void _get_skin_section(float x, Shape *shape,
                              Former *tail_f, vec3 *tail_l_tangents, vec3 tail_o_p, bool t_is_merge,
                              Former *nose_f, vec3 *nose_l_tangents, vec3 nose_o_p, bool n_is_merge) {
    Curve *t_curves = tail_f->shape.curves;
    Curve *n_curves = nose_f->shape.curves;

    float min_x = tail_f->x;
    float max_x = nose_f->x;
    float dx = max_x - min_x;
    float dc = dx * (float)LONGITUDINAL_SMOOTHNESS;

    float t = (x - min_x) / dx;
    float smooth_t = SMOOTHSTEP(t);

    for (int i = 0; i < SHAPE_CURVES; ++i) {
        vec3 p1 = vec3(tail_f->x, (float)t_curves[i].x, (float)t_curves[i].y);
        vec3 c1 = p1 + tail_l_tangents[i] * dc;
        vec3 p2 = vec3(nose_f->x, (float)n_curves[i].x, (float)n_curves[i].y);
        vec3 c2 = p2 - nose_l_tangents[i] * dc;

        vec3 p = bezier(p1, c1, c2, p2, t);
        shape->curves[i].x = p.y;
        shape->curves[i].y = p.z;

        if (t_is_merge && n_is_merge) {
            if (t < TWO_SIDE_MERGE_DELAY)
                shape->curves[i].w = t_curves[i].w;
            else if (t < ONE_MINUS_TWO_SIDE_MERGE_DELAY)
                shape->curves[i].w = lerp_1d(t_curves[i].w, n_curves[i].w, SMOOTHSTEP((t - TWO_SIDE_MERGE_DELAY) / (ONE_MINUS_TWO_SIDE_MERGE_DELAY - TWO_SIDE_MERGE_DELAY)));
            else
                shape->curves[i].w = n_curves[i].w;
        }
        else if (t_is_merge) {
            if (t < ONE_SIDE_MERGE_DELAY)
                shape->curves[i].w = t_curves[i].w;
            else
                shape->curves[i].w = lerp_1d(t_curves[i].w, n_curves[i].w, SMOOTHSTEP((t - ONE_SIDE_MERGE_DELAY) / ONE_MINUS_ONE_SIDE_MERGE_DELAY));
        }
        else if (n_is_merge) {
            if (t < ONE_MINUS_ONE_SIDE_MERGE_DELAY)
                shape->curves[i].w = lerp_1d(t_curves[i].w, n_curves[i].w, SMOOTHSTEP(t / ONE_MINUS_ONE_SIDE_MERGE_DELAY));
            else
                shape->curves[i].w = n_curves[i].w;
        }
        else
            shape->curves[i].w = lerp_1d(t_curves[i].w, n_curves[i].w, smooth_t);
    }

    shape_update_curve_control_points(shape->curves);
}

static inline bool _intersects_object(float x, Object *o) {
    return x >= o->min_x && x <= o->max_x;
}

static inline bool _intersects_connection(float x, Object *t_o, Object *n_o) {
    return x > t_o->max_x && x < n_o->min_x;
}

/* Marks position of fuselage sections. */
struct _Station {
    float x;
    OriginFlag t_objs; /* objects for which this station is tailmost */
    OriginFlag n_objs; /* objects for which this station is nosemost */
};

/* Initialize station. */
static void _init_station(_Station *s, float x, OriginFlag t_objs, OriginFlag n_objs) {
    s->x = x;
    s->t_objs = t_objs;
    s->n_objs = n_objs;
}

/* Insert new station sorted by x. If station with same x is found don't insert new station,
just update the old one with new object flags. */
static int _insert_station(_Station *stations, int count, float x, OriginFlag t_objs, OriginFlag n_objs) {
    int i = 0;
    for (; i < count; ++i)
        if (x <= stations[i].x)
            break;

    _Station *s = stations + i;

    if (s->x == x) {
        s->t_objs |= t_objs;
        s->n_objs |= n_objs;
    }
    else {
        for (int j = count; j > i; --j)
            stations[j] = stations[j - 1];
        _init_station(s, x, t_objs, n_objs);
        ++count;
    }

    return count;
}

/* Fuselage section at specific station. Contains all object and connection section shapes
and the resulting envelopes. Only two exist in memory at any time. */
struct _Section {
    Shape shapes[MAX_ENVELOPE_SHAPES]; /* actual storage */
    int shapes_count;
    Shape *t_shapes[MAX_ENVELOPE_SHAPES]; /* aliases, shapes looking tailwise */
    int t_shapes_count;
    Shape *n_shapes[MAX_ENVELOPE_SHAPES]; /* aliases, shapes looking nosewise */
    int n_shapes_count;
    MeshEnv envs[2]; /* actual storage */
    MeshEnv *t_env, *n_env; /* aliases of the above */
    int neighbors_map[MAX_ENVELOPE_POINTS]; /* maps tailwise envelope point indices to tailwise triangles */
};

/* Main fuselage lofting function. Generates fuselage skin panels. */
void fuselage_loft(Arena *arena, Arena *verts_arena, Model *model, Fuselage *fuselage) {
    _update_longitudinal_tangents(fuselage);

    int shape_subdivs = SHAPE_CURVE_SAMPLES * SHAPE_CURVES;

    /* set object origins and get fuselage extents */

    float min_x = FLT_MAX, max_x = -FLT_MAX;
    float min_y = FLT_MAX, max_y = -FLT_MAX;
    float min_z = FLT_MAX, max_z = -FLT_MAX;

    for (int i = 0; i < fuselage->objects_count; ++i) {
        Objref *o_ref = fuselage->objects + i;
        Object *o = o_ref->object;
        o_ref->origin = i;

        if (o->min_x < min_x)
            min_x = o->min_x;
        if (o->min_y < min_y)
            min_y = o->min_y;
        if (o->min_z < min_z)
            min_z = o->min_z;

        if (o->max_x > max_x)
            max_x = o->max_x;
        if (o->max_y > max_y)
            max_y = o->max_y;
        if (o->max_z > max_z)
            max_z = o->max_z;
    }

    /* estimate mesh size along x */

    float mesh_size = ((max_y - min_y) + (max_z - min_z)) * 2.0f / shape_subdivs;

    /* find all the fixed section positions (stations) */

    _Station *stations1 = arena->alloc<_Station>(MAX_FUSELAGE_OBJECTS * 2);
    int stations1_count = 0;

    for (int i = 0; i < fuselage->objects_count; ++i) {
        Objref *o_ref = fuselage->objects + i;
        Object *o = o_ref->object;

        if (o_ref->t_conns_count == 0)
            stations1_count = _insert_station(stations1, stations1_count,
                                              o->min_x, origin_index_to_flag(o_ref->origin), ZERO_ORIGIN_FLAG);

        if (o_ref->n_conns_count == 0)
            stations1_count = _insert_station(stations1, stations1_count,
                                              o->max_x, ZERO_ORIGIN_FLAG, origin_index_to_flag(o_ref->origin));
    }

    // TODO: merge stations that are too close, use mesh size to estimate

    model_assert(model, stations1_count >= 2, "stations_count_less_than_2");

    /* create all section positions */

    const static int MAX_STATIONS = 1000;
    _Station *stations2 = arena->alloc<_Station>(MAX_STATIONS);
    int stations2_count = 1;

    stations2[0] = stations1[0];

    for (int i = 1; i < stations1_count; ++i) {
        _Station *s1 = stations1 + i - 1;
        _Station *s2 = stations1 + i;

        float dx = s2->x - s1->x;
        int count = (int)floorf(dx / mesh_size); // TODO: round
        float ddx = dx / (count + 1);
        for (int j = 0; j < count; ++j) {
            break_assert(stations2_count < MAX_STATIONS);
            _Station *s = stations2 + stations2_count++;
            _init_station(s, s1->x + (j + 1) * ddx, ZERO_ORIGIN_FLAG, ZERO_ORIGIN_FLAG);
        }

        stations2[stations2_count++] = stations1[i];
    }

    int beg_section = 0;
    int end_section = stations2_count - 1;

    /* get fuselage section envelopes */

    _Section *sections[2];

    sections[0] = arena->alloc<_Section>();
    sections[1] = arena->alloc<_Section>();

    for (int i = 0; i < stations2_count; ++i) {
        _Section *section = sections[i % 2];
        _Station *station = stations2 + i;
        float section_x = station->x;

        section->shapes_count = 0;
        section->t_shapes_count = 0;
        section->n_shapes_count = 0;

        bool two_envelopes = false;

        /* intersect objects */

        for (int j = 0; j < fuselage->objects_count; ++j) {
            Objref *o_ref = fuselage->objects + j;
            Object *o = o_ref->object;
            OriginFlag flag = origin_index_to_flag(o_ref->origin);

            if (_intersects_object(section_x, o)) {
                break_assert(section->shapes_count < MAX_ENVELOPE_SHAPES);
                Shape *s = section->shapes + section->shapes_count++;

                bool is_tailmost = (i != beg_section) && (station->t_objs & flag);
                bool is_nosemost = (i != end_section) && (station->n_objs & flag);

                _get_skin_section(section_x, s,
                                  &o_ref->t_skin_former, o_ref->t_tangents, o->p, false,
                                  &o_ref->n_skin_former, o_ref->n_tangents, o->p, false);

                s->origin.tail = o_ref->origin;
                s->origin.nose = o_ref->origin;
                if (!is_tailmost)
                    section->t_shapes[section->t_shapes_count++] = s;
                if (!is_nosemost)
                    section->n_shapes[section->n_shapes_count++] = s;
                if (is_tailmost || is_nosemost)
                    two_envelopes = true;
            }
        }

        /* intersect connections between objects */

        for (int j = 0; j < fuselage->conns_count; ++j) {
            Conn *c = fuselage->conns + j;
            Objref *tail_o_ref = c->tail_o;
            Objref *nose_o_ref = c->nose_o;
            Object *tail_o = tail_o_ref->object;
            Object *nose_o = nose_o_ref->object;

            if (_intersects_connection(section_x, tail_o_ref->object, nose_o_ref->object)) {
                break_assert(section->shapes_count < MAX_ENVELOPE_SHAPES);
                Shape *s = section->shapes + section->shapes_count++;

                _get_skin_section(section_x, s,
                                  &tail_o_ref->n_skin_former, tail_o_ref->n_tangents, tail_o->p, tail_o_ref->n_conns_count > 1,
                                  &nose_o_ref->t_skin_former, nose_o_ref->t_tangents, nose_o->p, nose_o_ref->t_conns_count > 1);

                s->origin.tail = tail_o_ref->origin;
                s->origin.nose = nose_o_ref->origin;
                section->t_shapes[section->t_shapes_count++] = s;
                section->n_shapes[section->n_shapes_count++] = s;
            }
        }

        if (section->t_shapes_count == 0 || section->n_shapes_count == 0) /* skip if there are no shapes on either side */
            continue;

        /* trace envelopes */

        if (two_envelopes) {
            section->t_env = section->envs;
            section->n_env = section->envs + 1;
        }
        else
            section->t_env = section->n_env = section->envs;

        mesh_make_envelopes(model, arena, verts_arena, section_x,
                            section->t_env, section->t_shapes, section->t_shapes_count,
                            section->n_env, section->n_shapes, section->n_shapes_count);

        /* mesh */

        if (i == 0)
            for (int j = 0; j < section->n_env->count; ++j)
                section->neighbors_map[j] = -1; /* there are no triangles tailwise of the tailmost section */
        else {
            _Section *t_s = sections[(section == sections[0]) ? 1 : 0];
            _Section *n_s = section;

            mesh_apply_merge_filter(arena, shape_subdivs,
                                    t_s->n_shapes, t_s->n_shapes_count, t_s->n_env,
                                    n_s->t_shapes, n_s->t_shapes_count, n_s->t_env);

            mesh_between_two_sections(model, shape_subdivs,
                                      t_s->n_env, t_s->neighbors_map,
                                      n_s->t_env, n_s->neighbors_map);
        }
    }
}
