#include "ui_warehouse.h"
#include "modeling_object.h"
#include "modeling_wing.h"
#include "math_vec.h"
#include "platform.h"
#include <string.h>
#include <assert.h>


Warehouse warehouse;

static void _object_proto_init(OProto *proto, const char *name, float tail_endp_dx, float nose_endp_dx) {
    assert(tail_endp_dx > 0.01);
    assert(nose_endp_dx > 0.01);
    assert(strlen(name) < MAX_PROTO_NAME);
#ifdef PLATFORM_WIN
    strcpy_s(proto->name, name);
#else
    strcpy(proto->name, name);
#endif
    proto->def.formers_count = 0;
    proto->def.t_endp_dx = tail_endp_dx;
    proto->def.n_endp_dx = nose_endp_dx;
}

static void _object_proto_add_coll_former(OProto *proto, Shape shape, float x) {
    assert(proto->def.formers_count == 0 || x > proto->def.formers[proto->def.formers_count - 1].x); /* new former must be in front of the old one */
    Former *f = proto->def.formers + proto->def.formers_count++;
    f->shape = shape;
    f->x = x;
}

static void _object_proto_set_skin_formers(OProto *proto, Shape tail_shape, float tail_x, Shape nose_shape, float nose_x) {
    proto->def.t_skin_former.shape = tail_shape;
    proto->def.t_skin_former.x = tail_x;
    proto->def.n_skin_former.shape = nose_shape;
    proto->def.n_skin_former.x = nose_x;
}

static void _init_wing_proto(WProto *proto, const char *name,
                             Airfoil r_airfoil, float r_aoa, float r_chord, float r_x, float r_y, float r_z,
                             Airfoil t_airfoil, float t_aoa, float t_chord, float t_x, float t_y, float t_z) {
    proto->def.r_former.airfoil = r_airfoil;
    proto->def.r_former.aoa = r_aoa;
    proto->def.r_former.chord = r_chord;
    proto->def.r_former.x = r_x;
    proto->def.r_former.y = r_y;
    proto->def.r_former.z = r_z;

    proto->def.t_former.airfoil = t_airfoil;
    proto->def.t_former.aoa = t_aoa;
    proto->def.t_former.chord = t_chord;
    proto->def.t_former.x = t_x;
    proto->def.t_former.y = t_y;
    proto->def.t_former.z = t_z;

    proto->def.spars_count = 0;
}

Warehouse::Warehouse() {
    o_protos_count = 0;
    w_protos_count = 0;
    selected_proto = 0;
    is_object = true;
    is_open = false;

    /* object prototypes */

    OProto *klimov_vk1 = o_protos + o_protos_count++;
    _object_proto_init(klimov_vk1, "Klimov Vk-1", 0.5, 1.0);
    _object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.25), 0.0f);
    _object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.3), 1.8f);
    _object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.5), 2.4f);
    _object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.5), 2.7f);
    _object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.25), 3.0f);
    _object_proto_set_skin_formers(klimov_vk1,
                          shape_circle(0.0, 0.0, 0.28), 0.0f,
                          shape_circle(0.0, 0.0, 0.6), 3.0f);

    OProto *tumansky_r13 = o_protos + o_protos_count++;
    _object_proto_init(tumansky_r13, "Tumansky R-13", 2.0, 2.0);
    _object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.45), 0.0f);
    _object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.45), 1.4f);
    _object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.4), 1.8f);
    _object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.4), 2.2f);
    _object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.5), 2.8f);
    _object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.5), 3.7f);
    _object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.4), 4.0f);
    _object_proto_set_skin_formers(tumansky_r13,
                          shape_circle(0.0, 0.0, 0.5), 0.0f,
                          shape_circle(0.0, 0.0, 0.5), 4.0f);

    OProto *mirage_iii_intake = o_protos + o_protos_count++;
    _object_proto_init(mirage_iii_intake, "Mirage III intake", 2.0, 2.0);
    _object_proto_add_coll_former(mirage_iii_intake, shape_right_semi_circle(0.0, 0.0, 0.4), 0.0f);
    _object_proto_add_coll_former(mirage_iii_intake, shape_right_semi_circle(0.0, 0.0, 0.4), 0.4f);
    _object_proto_set_skin_formers(mirage_iii_intake,
                          shape_right_semi_circle(0.0, 0.0, 0.42), 0.0f,
                          shape_right_semi_circle(0.0, 0.0, 0.42), 0.4f);

    OProto *mig_15_cockpit = o_protos + o_protos_count++;
    _object_proto_init(mig_15_cockpit, "Mig-15 cockpit", 2.0, 2.0);
    _object_proto_add_coll_former(mig_15_cockpit, shape_rect(0.0, 0.0, 0.7, 1.5), 0.0f);
    _object_proto_add_coll_former(mig_15_cockpit, shape_rect(0.0, 0.0, 0.7, 1.5), 1.5f);
    _object_proto_set_skin_formers(mig_15_cockpit,
                          shape_rect(0.0, 0.0, 0.72, 1.52), 0.0f,
                          shape_rect(0.0, 0.0, 0.72, 1.52), 1.5f);

    OProto *nose_endpoint = o_protos + o_protos_count++;
    _object_proto_init(nose_endpoint, "Nose end-point", 2.0f, 0.05f);
    _object_proto_add_coll_former(nose_endpoint, shape_circle(0.0, 0.0, 0.1), 0.0f);
    _object_proto_add_coll_former(nose_endpoint, shape_circle(0.0, 0.0, 0.02), 0.2f);
    _object_proto_set_skin_formers(nose_endpoint,
                          shape_circle(0.0, 0.0, 0.1), 0.0f,
                          shape_circle(0.0, 0.0, 0.02), 0.2f);


    /* wing prototypes */

    WProto *mirage_iii_wing = w_protos + w_protos_count;
    _init_wing_proto(mirage_iii_wing, "Mirage III wing",
                     airfoils_base[0], 0.0f, 10.0f, 0.0f, 0.0f, 0.0f,
                     airfoils_base[0], 0.0f, 0.5f, -9.4f, 7.0f, 0.0f);
}

void Warehouse::open() {
    is_open = true;
    update_drawing_geometry();
}

void Warehouse::close() {
    is_open = false;
}

void Warehouse::select_next_part() {
    if (is_object) {
        ++selected_proto;
        if (selected_proto > o_protos_count - 1) {
            is_object = false;
            selected_proto = 0;
        }
    }
    else {
        ++selected_proto;
        if (selected_proto > w_protos_count - 1) {
            is_object = true;
            selected_proto = 0;
        }
    }
    update_drawing_geometry();
}

void Warehouse::select_prev_part() {
    if (is_object) {
        --selected_proto;
        if (selected_proto < 0) {
            is_object = false;
            selected_proto = o_protos_count - 1;
        }
    }
    else {
        --selected_proto;
        if (selected_proto < 0) {
            is_object = true;
            selected_proto = w_protos_count - 1;
        }
    }
    update_drawing_geometry();
}

Object *warehouse_make_selected_object(vec3 camera_pos, vec3 camera_dir) {
    Object *o = new Object();
    o->p = camera_pos + camera_dir * 10;
    o->def = warehouse.o_protos[warehouse.selected_proto].def;
    object_finish(o);
    warehouse.close();
    return o;
}

// Wing *warehouse_make_selected_wing(Warehouse *wh, vec3 camera_pos, vec3 camera_dir) {
//     vec3 p = camera_pos + camera_dir * 10;
//     Wing *w = wing_make_from_selected_base_airfoil(wh->selected_wpro, p.x, p.y, p.z);
//     wh->close();
//     return w;
// }
