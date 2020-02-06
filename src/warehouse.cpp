#include "warehouse.h"
#include "shape.h"
#include <string.h>
#include <assert.h>


Warehouse warehouse;

Warehouse::Warehouse() : selected_part(0), is_open(false) {

    /* object parts */

    Part &klimov_vk1 = parts.add();
    part_init(&klimov_vk1, "Klimov Vk-1", 0.5, 1.0, false, false);
    part_add_coll_former(&klimov_vk1, shape_circle(0.0, 0.0, 0.25), 0.0);
    part_add_coll_former(&klimov_vk1, shape_circle(0.0, 0.0, 0.3), 1.8);
    part_add_coll_former(&klimov_vk1, shape_circle(0.0, 0.0, 0.5), 2.4);
    part_add_coll_former(&klimov_vk1, shape_circle(0.0, 0.0, 0.5), 2.7);
    part_add_coll_former(&klimov_vk1, shape_circle(0.0, 0.0, 0.25), 3.0);
    part_set_skin_formers(&klimov_vk1,
                          shape_circle(0.0, 0.0, 0.28), 0.0,
                          shape_circle(0.0, 0.0, 0.6), 3.0);

    Part &tumansky_r13 = parts.add();
    part_init(&tumansky_r13, "Tumansky R-13", 2.0, 2.0, true, false);
    part_add_coll_former(&tumansky_r13, shape_circle(0.0, 0.0, 0.45), 0.0);
    part_add_coll_former(&tumansky_r13, shape_circle(0.0, 0.0, 0.45), 1.4);
    part_add_coll_former(&tumansky_r13, shape_circle(0.0, 0.0, 0.4), 1.8);
    part_add_coll_former(&tumansky_r13, shape_circle(0.0, 0.0, 0.4), 2.2);
    part_add_coll_former(&tumansky_r13, shape_circle(0.0, 0.0, 0.5), 2.8);
    part_add_coll_former(&tumansky_r13, shape_circle(0.0, 0.0, 0.5), 3.7);
    part_add_coll_former(&tumansky_r13, shape_circle(0.0, 0.0, 0.4), 4.0);
    part_set_skin_formers(&tumansky_r13,
                          shape_circle(0.0, 0.0, 0.5), 0.0,
                          shape_circle(0.0, 0.0, 0.5), 4.0);

    Part &mirage_iii_intake = parts.add();
    part_init(&mirage_iii_intake, "Mirage III intake", 2.0, 2.0, false, true);
    part_add_coll_former(&mirage_iii_intake, shape_right_semi_circle(0.0, 0.0, 0.4), 0.0);
    part_add_coll_former(&mirage_iii_intake, shape_right_semi_circle(0.0, 0.0, 0.4), 0.4);
    part_set_skin_formers(&mirage_iii_intake,
                          shape_right_semi_circle(0.0, 0.0, 0.42), 0.0,
                          shape_right_semi_circle(0.0, 0.0, 0.42), 0.4);

    // Part &f16_intake = parts.add();
    // part_init(&f16_intake, "F-16 intake", 2.0, 2.0, false, true);
    // part_add_coll_former(&f16_intake, shape_rounded_quad(0.75, 0.4, 0.7, 0.3, 0.0, 0.0), 0.0);
    // part_add_coll_former(&f16_intake, shape_rounded_quad(0.75, 0.4, 0.7, 0.3, 0.0, 0.0), 0.4);
    // part_add_skin_former(&f16_intake, shape_rounded_quad(0.8, 0.45, 0.7, 0.3, 0.0, 0.0), 0.0);
    // part_add_skin_former(&f16_intake, shape_rounded_quad(0.8, 0.45, 0.7, 0.3, 0.0, 0.0), 0.4);

    Part &mig_15_cockpit = parts.add();
    part_init(&mig_15_cockpit, "Mig-15 cockpit", 2.0, 2.0, false, false);
    part_add_coll_former(&mig_15_cockpit, shape_rect(0.0, 0.0, 0.7, 1.5), 0);
    part_add_coll_former(&mig_15_cockpit, shape_rect(0.0, 0.0, 0.7, 1.5), 1.5);
    part_set_skin_formers(&mig_15_cockpit,
                          shape_rect(0.0, 0.0, 0.72, 1.52), 0.0,
                          shape_rect(0.0, 0.0, 0.72, 1.52), 1.5);

    Part &nose_endpoint = parts.add();
    part_init(&nose_endpoint, "Nose end-point", 2.0, 0.05, false, true);
    part_add_coll_former(&nose_endpoint, shape_circle(0.0, 0.0, 0.1), 0);
    part_add_coll_former(&nose_endpoint, shape_circle(0.0, 0.0, 0.02), 0.2);
    part_set_skin_formers(&nose_endpoint,
                          shape_circle(0.0, 0.0, 0.1), 0.0,
                          shape_circle(0.0, 0.0, 0.02), 0.2);

    // Part &lancair_cockpit = parts.add();
    // part_init(&lancair_cockpit, "Lancair cockpit", 0.1, 2.0, false, false);
    // part_add_coll_former(&lancair_cockpit, shape_make_symmetric(0.0, 0.0, 0.37,  0.35,    SHAPE_W_CIRCLE, 0.35,  SHAPE_W_CIRCLE), 0.0); // cockpit tail end
    // part_add_coll_former(&lancair_cockpit, shape_make_symmetric(0.0, 0.0, 0.45,  0.55,    SHAPE_W_CIRCLE, 0.55,  2.0), 0.81); // canopy top
    // part_add_coll_former(&lancair_cockpit, shape_make_symmetric(0.0, -0.13, 0.475, 0.363, SHAPE_W_CIRCLE, 0.363, 2.0), 2.44); // firewall
    // part_add_skin_former(&lancair_cockpit, shape_make_symmetric(0.0, 0.0, 0.37,  0.35,    SHAPE_W_CIRCLE, 0.35,  SHAPE_W_CIRCLE), 0.0); // cockpit tail end
    // part_add_skin_former(&lancair_cockpit, shape_make_symmetric(0.0, 0.0, 0.45,  0.55,    SHAPE_W_CIRCLE, 0.55,  2.0), 0.81); // canopy top
    // part_add_skin_former(&lancair_cockpit, shape_make_symmetric(0.0, -0.13, 0.475, 0.363, SHAPE_W_CIRCLE, 0.363, 2.0), 2.44); // firewall

    // Part *lancair_tail = &parts.add();
    // part_init(lancair_tail, "Lancair tail", 2.0, 2.0, false, true);
    // part_add_coll_former(lancair_tail, shape_rounded_quad(0.05, 0.3, 0.5, 0.5, 0.0, 0.0), 0.0);
    // part_add_coll_former(lancair_tail, shape_rounded_quad(0.05, 0.3, 0.5, 0.5, 0.0, 0.0), 0.3);
    // part_add_skin_former(lancair_tail, shape_rounded_quad(0.05, 0.3, 0.5, 0.5, 0.0, 0.0), 0.0);
    // part_add_skin_former(lancair_tail, shape_rounded_quad(0.05, 0.3, 0.5, 0.5, 0.0, 0.0), 0.3);
}

void Warehouse::open() {
    is_open = true;
    update_drawing_geometry();
}

void Warehouse::close() {
    is_open = false;
}

void Warehouse::select_next_part() {
    selected_part = ++selected_part;
    if (selected_part > parts.count - 1)
        selected_part = 0;
    update_drawing_geometry();
}

void Warehouse::select_prev_part() {
    selected_part = --selected_part;
    if (selected_part < 0)
        selected_part = parts.count - 1;
    update_drawing_geometry();
}

Object *Warehouse::make_selected_part(vec3 camera_pos, vec3 camera_dir) {
    Object *o = part_make_object(&parts[selected_part], camera_pos + camera_dir * 10);
    close();
    return o;
}
