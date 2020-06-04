#include "fuselage.h"
#include "object.h"
#include "wing.h"
#include "../interp.h"
#include "../periodic.h"
#include "mesh.h"
#include "../arena.h"
#include "../debug.h"
#include <math.h>
#include <float.h>

#define _MAX_FUSELAGE_STATIONS 1000


/* Marks position of fuselage sections. */
struct _Station {
    float x;
    short int id;
};

static void _init_station(_Station *s, float x, int id) {
    s->x = x;
    s->id = id;
}

/* Insert new station sorted by x and return its id. If station with same x is
found don't insert a new station, just return id of the old one. */
static int _insert_station(_Station *stations, int *count, float x) {
    int i = 0;
    _Station *s = 0;

    for (; i < *count; ++i)
        if (x <= stations[i].x) {
            s = stations + i;
            break;
        }

    if (s && s->x == x) /* found station at same x */
        return s->id;
    else { /* insert new station at i */
        for (int j = *count; j > i; --j)
            stations[j] = stations[j - 1];
        int id = *count;
        _init_station(stations + i, x, id);
        ++(*count);
        return id;
    }
}

/* Returns a shape that represents a section of object or connection (pair of
formers with associated longitudinal tangents). */
static void _get_section_shape(float x, Shape *shape,
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

// TODO: does this have to be a separate function?
void _get_shapes_at_station(Fuselage *fuselage,
                            _Station *station,
                            TraceShapes *shapes,
                            short int tailmost_station_id,
                            short int nosemost_station_id) {

    shapes->shapes_count = 0;
    shapes->t_shapes_count = 0;
    shapes->n_shapes_count = 0;
    shapes->two_envelopes = false;

    /* intersect objects */

    for (int i = 0; i < fuselage->orefs_count; ++i) {
        Oref *oref = fuselage->orefs + i;

        if (station->x >= oref->object->min_x && station->x <= oref->object->max_x) {
            break_assert(shapes->shapes_count < MAX_ENVELOPE_SHAPES);
            Shape *s = shapes->shapes + shapes->shapes_count++;

            bool is_t_opening = station->id != tailmost_station_id && station->id == oref->t_station.id;
            bool is_n_opening = station->id != nosemost_station_id && station->id == oref->n_station.id;

            _get_section_shape(station->x, s,
                               &oref->t_skin_former, oref->t_tangents, false,
                               &oref->n_skin_former, oref->n_tangents, false);

            s->ids.tail = oref->id;
            s->ids.nose = oref->id;
            if (!is_t_opening)
                shapes->t_shapes[shapes->t_shapes_count++] = s;
            if (!is_n_opening)
                shapes->n_shapes[shapes->n_shapes_count++] = s;
            if (is_t_opening || is_n_opening)
                shapes->two_envelopes = true;
        }
    }

    /* intersect connections between objects */

    for (int i = 0; i < fuselage->conns_count; ++i) {
        Conn *c = fuselage->conns + i;
        Oref *t_oref = c->tail_o;
        Oref *n_oref = c->nose_o;

        if (station->x > t_oref->object->max_x && station->x < n_oref->object->min_x) {
            break_assert(shapes->shapes_count < MAX_ENVELOPE_SHAPES);
            Shape *s = shapes->shapes + shapes->shapes_count++;

            _get_section_shape(station->x, s,
                               &t_oref->n_skin_former, t_oref->n_tangents, t_oref->n_conns_count > 1,
                               &n_oref->t_skin_former, n_oref->t_tangents, n_oref->t_conns_count > 1);

            s->ids.tail = t_oref->id;
            s->ids.nose = n_oref->id;
            shapes->t_shapes[shapes->t_shapes_count++] = s;
            shapes->n_shapes[shapes->n_shapes_count++] = s;
        }
    }
}

/* Fuselage section containing mesh envelopes. */
struct _MeshSection {
    MeshEnv envs[2]; /* actual storage */
    MeshEnv *t_env, *n_env; /* aliases of the above, both point envs[0] most of the time */
    int neighbors_map[MAX_ENVELOPE_POINTS]; /* maps tailwise envelope point indices to tailwise triangles */
};

/* Main fuselage lofting function. Generates fuselage skin panels. */
void fuselage_loft(Arena *arena, Arena *verts_arena,
                   Model *model, Fuselage *fuselage) {

    int shape_subdivs = SHAPE_CURVE_SAMPLES * SHAPE_CURVES;

    /* assign fuselage elements' ids */

    {
        int next_elem_id = 0;

        for (int i = 0; i < fuselage->orefs_count; ++i) {
            Oref *oref = fuselage->orefs + i;
            oref->id = next_elem_id++;
            oref->t_station.id = -1;
            oref->n_station.id = -1;
        }

        for (int i = 0; i < fuselage->wrefs_count; ++i) {
            Wref *wref = fuselage->wrefs + i;
            wref->id = next_elem_id++;
            wref->t_station.id = -1;
            wref->n_station.id = -1;
        }
    }

    /* get required stations */

    _Station *req_stations = arena->alloc<_Station>(MAX_ELEM_REFS * 2);
    int req_stations_count = 0;

    {
        /* object required stations (openings) */

        for (int i = 0; i < fuselage->orefs_count; ++i) {
            Oref *oref = fuselage->orefs + i;
            Object *o = oref->object;

            if (oref->t_conns_count == 0) /* tailwise opening */
                oref->t_station.id = _insert_station(req_stations,
                                                     &req_stations_count,
                                                     o->min_x);

            if (oref->n_conns_count == 0) /* nosewise opening */
                oref->n_station.id = _insert_station(req_stations,
                                                     &req_stations_count,
                                                     o->max_x);
        }

        /* get wing required stations (leading and trailing edge root points, spars) */

        for (int i = 0; i < fuselage->wrefs_count; ++i) {
            Wref *wref = fuselage->wrefs + i;

            static float x_positions[MAX_ELEM_REFS];
            int count = wing_get_required_stations(wref->wing, x_positions);

            /* trailing edge */
            wref->t_station.id = _insert_station(req_stations,
                                                 &req_stations_count,
                                                 x_positions[0]);

            //
            // TODO: add spar stations
            //

            /* leading edge */
            wref->n_station.id = _insert_station(req_stations,
                                                 &req_stations_count,
                                                 x_positions[count - 1]);
        }

        // TODO: merge stations that are too close along x, use mesh size to estimate
    }

    /* approximate mesh size */

    float mesh_size = 0.0f;

    {
        float min_y = FLT_MAX, max_y = -FLT_MAX;
        float min_z = FLT_MAX, max_z = -FLT_MAX;

        for (int i = 0; i < fuselage->orefs_count; ++i) {
            Oref *oref = fuselage->orefs + i;
            Object *o = oref->object;
            if (o->min_y < min_y)
                min_y = o->min_y;
            if (o->min_z < min_z)
                min_z = o->min_z;
            if (o->max_y > max_y)
                max_y = o->max_y;
            if (o->max_z > max_z)
                max_z = o->max_z;
        }

        mesh_size = ((max_y - min_y) + (max_z - min_z)) * 2.0f / shape_subdivs;
    }

    /* once we have required stations we can create actual stations
    by copying required ones and inserting additional stations wrt
    calculated mesh size */

    _Station *stations = arena->alloc<_Station>(_MAX_FUSELAGE_STATIONS);
    int stations_count = 0;
    short int tailmost_station_id;
    short int nosemost_station_id;

    {
        short int *id_to_index = arena->alloc<short int>(MAX_ELEM_REFS * 2); /* maps stations id to index */

        /* copy first required station */

        stations[stations_count++] = req_stations[0];
        id_to_index[req_stations[0].id] = 0;
        short int available_station_id = req_stations_count;

        for (int i = 1; i < req_stations_count; ++i) {
            _Station *s1 = req_stations + i - 1;
            _Station *s2 = req_stations + i;

            /* insert additional stations */

            float d = s2->x - s1->x;
            int count = (int)floorf(d / mesh_size);
            float dx = d / (count + 1);

            for (int j = 0; j < count; ++j) {
                break_assert(stations_count < _MAX_FUSELAGE_STATIONS);
                _init_station(stations + stations_count++,
                              s1->x + (j + 1) * dx,
                              available_station_id++);
            }

            /* copy required station */

            id_to_index[req_stations[i].id] = stations_count;
            stations[stations_count++] = req_stations[i];
        }

        tailmost_station_id = stations[0].id;
        nosemost_station_id = stations[stations_count - 1].id;

        /* map oref and wref station ids to indices */

        for (int i = 0; i < fuselage->orefs_count; ++i) {
            Oref *oref = fuselage->orefs + i;
            oref->t_station.index = id_to_index[oref->t_station.id];
            oref->n_station.index = id_to_index[oref->n_station.id];
        }
        for (int i = 0; i < fuselage->wrefs_count; ++i) {
            Wref *wref = fuselage->wrefs + i;
            wref->t_station.index = id_to_index[wref->t_station.id];
            wref->n_station.index = id_to_index[wref->n_station.id];
        }
    }

    /* trace fuselage section envelopes */

    TraceSection *trace_sections = arena->alloc<TraceSection>(stations_count);

    for (int i = 0; i < stations_count; ++i) {
        _Station *station = stations + i;

        TraceSection *sect = trace_sections + i;
        sect->wisecs_count = 0;
        sect->t_env = sect->n_env = 0;
        sect->x = station->x;

        _get_shapes_at_station(fuselage, station, &sect->shapes,
                               tailmost_station_id,
                               nosemost_station_id);

        if (sect->shapes.t_shapes_count == 0 || sect->shapes.n_shapes_count == 0) /* skip if there are no shapes on either side */
            continue;

        /* trace envelopes */

        sect->t_env = sect->n_env = arena->alloc<TraceEnv>();
        bool success = mesh_trace_envelope(sect->t_env, sect->shapes.t_shapes, sect->shapes.t_shapes_count, SHAPE_CURVE_SAMPLES);
        model_assert(model, success, "envelope_trace_failed");

        if (sect->shapes.two_envelopes) {
            sect->n_env = arena->alloc<TraceEnv>();
            bool n_success = mesh_trace_envelope(sect->n_env, sect->shapes.n_shapes, sect->shapes.n_shapes_count, SHAPE_CURVE_SAMPLES);
            model_assert(model, n_success, "envelope_trace_failed");
        }
    }

    /* wing intersections */

    fuselage_wing_intersections(arena,
                                fuselage->wrefs, fuselage->wrefs_count,
                                trace_sections, stations_count);

    /* mesh between each two neighboring sections */

    _MeshSection *sections[2];
    sections[0] = arena->alloc<_MeshSection>();
    sections[1] = arena->alloc<_MeshSection>();

    for (int i = 0; i < stations_count; ++i) {
        _Station *station = stations + i;
        _MeshSection *section = sections[i % 2];
        TraceSection *trace_section = trace_sections + i;
        TraceShapes *shapes = &trace_section->shapes;

        if (trace_section->t_env == 0 && trace_section->n_env == 0)
            continue;

        if (shapes->two_envelopes) {
            section->t_env = section->envs;
            section->n_env = section->envs + 1;
        }
        else
            section->t_env = section->n_env = section->envs;

        /* make mesh envelopes from trace envelopes */

        mesh_make_envelopes(model, verts_arena, station->x,
                            section->t_env, trace_section->t_env,
                            section->n_env, trace_section->n_env);

        /* mesh */

        if (i == 0)
            for (int j = 0; j < section->n_env->count; ++j)
                section->neighbors_map[j] = -1; /* there are no triangles tailwise of the tailmost section */
        else {
            _MeshSection *t_s = sections[(section == sections[0]) ? 1 : 0];
            _MeshSection *n_s = section;
            TraceShapes *t_shapes = &trace_sections[i - 1].shapes;
            TraceShapes *n_shapes = &trace_sections[i].shapes;

            mesh_apply_merge_filter(arena, shape_subdivs,
                                    t_shapes->n_shapes, t_shapes->n_shapes_count, t_s->n_env,
                                    n_shapes->t_shapes, n_shapes->t_shapes_count, n_s->t_env);

            mesh_between_two_sections(model, shape_subdivs,
                                      t_s->n_env, t_s->neighbors_map,
                                      n_s->t_env, n_s->neighbors_map);
        }
    }
}
