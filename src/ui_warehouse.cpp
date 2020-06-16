#include "ui_warehouse.h"
#include "modeling_object.h"
#include "modeling_wing.h"
#include "math_vec.h"
#include "platform.h"
#include <string.h>
#include <assert.h>


Warehouse warehouse;

Warehouse::Warehouse() {
    parts_count = 0;
    selected_part = 0;
    wpros_count = 0;
    selected_wpro = 0;
    mode = WM_CLOSED;

    /* object parts */

    ObjectProto *klimov_vk1 = parts + parts_count++;
    object_proto_init(klimov_vk1, "Klimov Vk-1", 0.5, 1.0);
    object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.25), 0.0f);
    object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.3), 1.8f);
    object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.5), 2.4f);
    object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.5), 2.7f);
    object_proto_add_coll_former(klimov_vk1, shape_circle(0.0, 0.0, 0.25), 3.0f);
    object_proto_set_skin_formers(klimov_vk1,
                          shape_circle(0.0, 0.0, 0.28), 0.0f,
                          shape_circle(0.0, 0.0, 0.6), 3.0f);

    ObjectProto *tumansky_r13 = parts + parts_count++;
    object_proto_init(tumansky_r13, "Tumansky R-13", 2.0, 2.0);
    object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.45), 0.0f);
    object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.45), 1.4f);
    object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.4), 1.8f);
    object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.4), 2.2f);
    object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.5), 2.8f);
    object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.5), 3.7f);
    object_proto_add_coll_former(tumansky_r13, shape_circle(0.0, 0.0, 0.4), 4.0f);
    object_proto_set_skin_formers(tumansky_r13,
                          shape_circle(0.0, 0.0, 0.5), 0.0f,
                          shape_circle(0.0, 0.0, 0.5), 4.0f);

    ObjectProto *mirage_iii_intake = parts + parts_count++;
    object_proto_init(mirage_iii_intake, "Mirage III intake", 2.0, 2.0);
    object_proto_add_coll_former(mirage_iii_intake, shape_right_semi_circle(0.0, 0.0, 0.4), 0.0f);
    object_proto_add_coll_former(mirage_iii_intake, shape_right_semi_circle(0.0, 0.0, 0.4), 0.4f);
    object_proto_set_skin_formers(mirage_iii_intake,
                          shape_right_semi_circle(0.0, 0.0, 0.42), 0.0f,
                          shape_right_semi_circle(0.0, 0.0, 0.42), 0.4f);

    ObjectProto *mig_15_cockpit = parts + parts_count++;
    object_proto_init(mig_15_cockpit, "Mig-15 cockpit", 2.0, 2.0);
    object_proto_add_coll_former(mig_15_cockpit, shape_rect(0.0, 0.0, 0.7, 1.5), 0.0f);
    object_proto_add_coll_former(mig_15_cockpit, shape_rect(0.0, 0.0, 0.7, 1.5), 1.5f);
    object_proto_set_skin_formers(mig_15_cockpit,
                          shape_rect(0.0, 0.0, 0.72, 1.52), 0.0f,
                          shape_rect(0.0, 0.0, 0.72, 1.52), 1.5f);

    ObjectProto *nose_endpoint = parts + parts_count++;
    object_proto_init(nose_endpoint, "Nose end-point", 2.0f, 0.05f);
    object_proto_add_coll_former(nose_endpoint, shape_circle(0.0, 0.0, 0.1), 0.0f);
    object_proto_add_coll_former(nose_endpoint, shape_circle(0.0, 0.0, 0.02), 0.2f);
    object_proto_set_skin_formers(nose_endpoint,
                          shape_circle(0.0, 0.0, 0.1), 0.0f,
                          shape_circle(0.0, 0.0, 0.02), 0.2f);
}

void Warehouse::open(bool wings) {
    mode = wings ? WM_WING : WM_OBJECT;
    update_drawing_geometry();
}

void Warehouse::close() {
    mode = WM_CLOSED;
}

void Warehouse::select_next_part() {
    if (mode == WM_OBJECT) {
        ++selected_part;
        if (selected_part > parts_count - 1)
            selected_part = 0;
    }
    else if (mode == WM_WING) {
        ++selected_wpro;
        if (selected_wpro > airfoils_base_count - 1)
            selected_wpro = 0;
    }
    update_drawing_geometry();
}

void Warehouse::select_prev_part() {
    if (mode == WM_OBJECT) {
        --selected_part;
        if (selected_part < 0)
            selected_part = parts_count - 1;
    }
    else if (mode == WM_WING) {
        --selected_wpro;
        if (selected_wpro < 0)
            selected_wpro = airfoils_base_count - 1;
    }
    update_drawing_geometry();
}

Object *Warehouse::make_selected_part(vec3 camera_pos, vec3 camera_dir) {
    Object *o = object_proto_make_object(&parts[selected_part], camera_pos + camera_dir * 10);
    close();
    return o;
}

Wing *warehouse_make_selected_wing(Warehouse *wh, vec3 camera_pos, vec3 camera_dir) {
    vec3 p = camera_pos + camera_dir * 10;
    Wing *w = wing_make_from_selected_base_airfoil(wh->selected_wpro, p.x, p.y, p.z);
    wh->close();
    return w;
}

void object_proto_init(ObjectProto *proto, const char *name, float tail_endp_dx, float nose_endp_dx) {
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

void object_proto_add_coll_former(ObjectProto *proto, Shape shape, float x) {
    assert(proto->def.formers_count == 0 || x > proto->def.formers[proto->def.formers_count - 1].x); /* new former must be in front of the old one */
    Former *f = proto->def.formers + proto->def.formers_count++;
    f->shape = shape;
    f->x = x;
}

void object_proto_set_skin_formers(ObjectProto *proto, Shape tail_shape, float tail_x, Shape nose_shape, float nose_x) {
    proto->def.t_skin_former.shape = tail_shape;
    proto->def.t_skin_former.x = tail_x;
    proto->def.n_skin_former.shape = nose_shape;
    proto->def.n_skin_former.x = nose_x;
}

Object *object_proto_make_object(ObjectProto *proto, vec3 p) {
    Object *o = new Object();
    o->p = p;
    o->def = proto->def;
    object_finish(o);
    return o;
}
