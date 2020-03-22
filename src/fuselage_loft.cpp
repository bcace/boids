#include "fuselage.h"
#include "object.h"
#include "wing.h"
#include "interp.h"
#include "mesh.h"
#include "arena.h"
#include "debug.h"
#include <math.h>
#include <float.h>


/* Returns a shape that represents a section of object or connection (pair of
formers with associated longitudinal tangents). */
static void _get_skin_section(float x, Shape *shape,
                              Former *t_form, tvec *t_tangents, bool t_is_merge,
                              Former *n_form, tvec *n_tangents, bool n_is_merge) {
    Curve *t_curves = t_form->shape.curves;
    Curve *n_curves = n_form->shape.curves;

    float min_x = t_form->x;
    float max_x = n_form->x;
    float dx = max_x - min_x;
    float dc = dx * (float)LONGITUDINAL_SMOOTHNESS;

    float t = (x - min_x) / dx;

    for (int i = 0; i < SHAPE_CURVES; ++i) {
        Curve *t_curve = t_curves + i;
        Curve *n_curve = n_curves + i;
        tvec t_tangent = t_tangents[i];
        tvec n_tangent = n_tangents[i];

        tvec p1, c1, c2, p2; /* create the bezier curve */
        p1.x = t_form->x;
        p1.y = t_curve->x;
        p1.z = t_curve->y;
        c1.x = p1.x + t_tangent.x * dc;
        c1.y = p1.y + t_tangent.y * dc;
        c1.z = p1.z + t_tangent.z * dc;
        p2.x = n_form->x;
        p2.y = n_curve->x;
        p2.z = n_curve->y;
        c2.x = p2.x - n_tangent.x * dc;
        c2.y = p2.y - n_tangent.y * dc;
        c2.z = p2.z - n_tangent.z * dc;

        tvec p = cube_bezier_3d(p1, c1, c2, p2, t); /* sample bezier curve */

        shape->curves[i].x = p.y;
        shape->curves[i].y = p.z;

        /* merge delay, delays interpolation of curve w parameter to keep shapes
        similar to each other if they're close together */

        float f1, f2;

        if (t_is_merge && n_is_merge) {
            f1 = TWO_SIDE_MERGE_DELAY;
            f2 = 1.0f - TWO_SIDE_MERGE_DELAY;
        }
        else if (t_is_merge) {
            f1 = ONE_SIDE_MERGE_DELAY;
            f2 = 1.0f;
        }
        else if (n_is_merge) {
            f1 = 0.0f;
            f2 = 1.0f - ONE_SIDE_MERGE_DELAY;
        }
        else {
            f1 = 0.0f;
            f2 = 1.0f;
        }

        if (t < f1)
            shape->curves[i].w = t_curve->w;
        else if (t <= f2)
            shape->curves[i].w = lerp_1d(t_curve->w, n_curve->w, SMOOTHSTEP((t - f1) / (f2 - f1)));
        else
            shape->curves[i].w = n_curve->w;
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
    Flags t_objs; /* objects for which this station is tailmost */
    Flags n_objs; /* objects for which this station is nosemost */
};

/* Initialize station. */
static void _init_station(_Station *s, float x, Flags t_objs, Flags n_objs) {
    s->x = x;
    s->t_objs = t_objs;
    s->n_objs = n_objs;
}

/* Insert new station sorted by x. If station with same x is found don't insert new station,
just update the old one with new object flags. */
static int _insert_station(_Station *stations, int count, float x, Flags t_objs, Flags n_objs) {
    int i = 0;
    _Station *s = 0;

    for (; i < count; ++i)
        if (x <= stations[i].x) {
            s = stations + i;
            break;
        }

    if (s && s->x == x) {
        flags_add_flags(&s->t_objs, &t_objs);
        flags_add_flags(&s->n_objs, &n_objs);
    }
    else {
        for (int j = count; j > i; --j)
            stations[j] = stations[j - 1];
        _init_station(stations + i, x, t_objs, n_objs);
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
    int shape_subdivs = SHAPE_CURVE_SAMPLES * SHAPE_CURVES;

    /* assign object id, get fuselage extents and get required stations */

    float min_x = FLT_MAX, max_x = -FLT_MAX;
    float min_y = FLT_MAX, max_y = -FLT_MAX;
    float min_z = FLT_MAX, max_z = -FLT_MAX;

    _Station *stations1 = arena->alloc<_Station>(MAX_ELEM_REFS * 2);
    int stations1_count = 0;

    for (int i = 0; i < fuselage->orefs_count; ++i) {
        Oref *oref = fuselage->orefs + i;
        Object *o = oref->object;

        oref->id = i; /* assigning object origin */

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

        /* required stations */

        if (oref->t_conns_count == 0) /* tailwise opening */
            stations1_count = _insert_station(stations1, stations1_count,
                                              o->min_x, flags_make(oref->id), flags_zero());

        if (oref->n_conns_count == 0) /* nosewise opening */
            stations1_count = _insert_station(stations1, stations1_count,
                                              o->max_x, flags_zero(), flags_make(oref->id));
    }

    /* TODO: assign wing id, get required stations */

    float w_stations[MAX_ELEM_REFS];

    for (int i = 0; i < fuselage->wrefs_count; ++i) {
        Wref *wref = fuselage->wrefs + i;
        Wing *w = wref->wing;
        Flags f = flags_make(wref->id);

        int count = wing_get_required_stations(w, w_stations);

        stations1_count = _insert_station(stations1, stations1_count,
                                          w_stations[0], f, flags_zero());

        stations1_count = _insert_station(stations1, stations1_count,
                                          w_stations[count - 1], flags_zero(), f);
    }

    // TODO: merge stations that are too close, use mesh size to estimate

    model_assert(model, stations1_count >= 2, "stations_count_less_than_2");

    /* create all section positions */

    const static int MAX_STATIONS = 1000;

    _Station *stations2 = arena->alloc<_Station>(MAX_STATIONS);
    int stations2_count = 1;

    stations2[0] = stations1[0];

    float mesh_size = ((max_y - min_y) + (max_z - min_z)) * 2.0f / shape_subdivs;

    for (int i = 1; i < stations1_count; ++i) {
        _Station *s1 = stations1 + i - 1;
        _Station *s2 = stations1 + i;

        float dx = s2->x - s1->x;
        int count = (int)floorf(dx / mesh_size); // TODO: round
        float ddx = dx / (count + 1);
        for (int j = 0; j < count; ++j) {
            break_assert(stations2_count < MAX_STATIONS);
            _Station *s = stations2 + stations2_count++;
            _init_station(s, s1->x + (j + 1) * ddx, flags_zero(), flags_zero());
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

        for (int j = 0; j < fuselage->orefs_count; ++j) {
            Oref *oref = fuselage->orefs + j;
            Object *o = oref->object;
            Flags flag = flags_make(oref->id);

            if (_intersects_object(section_x, o)) {
                break_assert(section->shapes_count < MAX_ENVELOPE_SHAPES);
                Shape *s = section->shapes + section->shapes_count++;

                bool is_tailmost = (i != beg_section) && flags_and(&station->t_objs, &flag);
                bool is_nosemost = (i != end_section) && flags_and(&station->n_objs, &flag);

                _get_skin_section(section_x, s,
                                  &oref->t_skin_former, oref->t_tangents, false,
                                  &oref->n_skin_former, oref->n_tangents, false);

                s->ids.tail = oref->id;
                s->ids.nose = oref->id;
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
            Oref *tail_o_ref = c->tail_o;
            Oref *nose_o_ref = c->nose_o;
            Object *tail_o = tail_o_ref->object;
            Object *nose_o = nose_o_ref->object;

            if (_intersects_connection(section_x, tail_o_ref->object, nose_o_ref->object)) {
                break_assert(section->shapes_count < MAX_ENVELOPE_SHAPES);
                Shape *s = section->shapes + section->shapes_count++;

                _get_skin_section(section_x, s,
                                  &tail_o_ref->n_skin_former, tail_o_ref->n_tangents, tail_o_ref->n_conns_count > 1,
                                  &nose_o_ref->t_skin_former, nose_o_ref->t_tangents, nose_o_ref->t_conns_count > 1);

                s->ids.tail = tail_o_ref->id;
                s->ids.nose = nose_o_ref->id;
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
